#include "file.hpp"
#include "eval.hpp"
#include "ast.hpp"
#include "bind.hpp"
#include "util.hpp"
#include "to_string.hpp"
#include "inspect.hpp"
#include "to_c.hpp"
#include "context.hpp"
#include "backtrace.hpp"
#include "prelexer.hpp"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <typeinfo>

namespace Sass {
  using namespace std;

  inline double add(double x, double y) { return x + y; }
  inline double sub(double x, double y) { return x - y; }
  inline double mul(double x, double y) { return x * y; }
  inline double div(double x, double y) { return x / y; } // x/0 checked by caller
  typedef double (*bop)(double, double);
  bop ops[Binary_Expression::NUM_OPS] = {
    0, 0, // and, or
    0, 0, 0, 0, 0, 0, // eq, neq, gt, gte, lt, lte
    add, sub, mul, div, fmod
  };

  Eval::Eval(Context& ctx, Env* env, Backtrace* bt)
  : ctx(ctx), env(env), backtrace(bt) { }
  Eval::~Eval() { }

  Eval* Eval::with(Env* e, Backtrace* bt) // for setting the env before eval'ing an expression
  {
    env = e;
    backtrace = bt;
    return this;
  }

  Expression* Eval::operator()(Block* b)
  {
    Expression* val = 0;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      val = (*b)[i]->perform(this);
      if (val) return val;
    }
    return val;
  }

  Expression* Eval::operator()(Assignment* a)
  {
    string var(a->variable());
    if (env->has(var)) {
      if(!a->is_guarded()) (*env)[var] = a->value()->perform(this);
    }
    else {
      env->current_frame()[var] = a->value()->perform(this);
    }
    return 0;
  }

  Expression* Eval::operator()(If* i)
  {
    if (*i->predicate()->perform(this)) {
      return i->consequent()->perform(this);
    }
    else {
      Block* alt = i->alternative();
      if (alt) return alt->perform(this);
    }
    return 0;
  }

  Expression* Eval::operator()(For* f)
  {
    string variable(f->variable());
    Expression* low = f->lower_bound()->perform(this);
    if (low->concrete_type() != Expression::NUMBER) {
      error("lower bound of `@for` directive must be numeric", low->path(), low->position());
    }
    Expression* high = f->upper_bound()->perform(this);
    if (high->concrete_type() != Expression::NUMBER) {
      error("upper bound of `@for` directive must be numeric", high->path(), high->position());
    }
    double start = static_cast<Number*>(low)->value();
    double end = static_cast<Number*>(high)->value();
    Env new_env;
    new_env[variable] = new (ctx.mem) Number(low->path(), low->position(), start);
    new_env.link(env);
    env = &new_env;
    Block* body = f->block();
    Expression* val = 0;
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start;
           i < end;
           (*env)[variable] = new (ctx.mem) Number(low->path(), low->position(), ++i)) {
        val = body->perform(this);
        if (val) break;
      }
    } else {
      if (f->is_inclusive()) --end;
      for (double i = start;
           i > end;
           (*env)[variable] = new (ctx.mem) Number(low->path(), low->position(), --i)) {
        val = body->perform(this);
        if (val) break;
      }
    }
    env = new_env.parent();
    return val;
  }

  Expression* Eval::operator()(Each* e)
  {
    vector<string> variables(e->variables());
    Expression* expr = e->list()->perform(this);
    List* list = 0;
    Map* map = 0;
    if (expr->concrete_type() == Expression::MAP) {
      map = static_cast<Map*>(expr);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = new (ctx.mem) List(expr->path(), expr->position(), 1, List::COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    Env new_env;
    for (size_t i = 0, L = variables.size(); i < L; ++i) new_env[variables[i]] = 0;
    new_env.link(env);
    env = &new_env;
    Block* body = e->block();
    Expression* val = 0;

    if (map) {
      for (auto key : map->keys()) {
        (*env)[variables[0]] = key;
        (*env)[variables[1]] = map->at(key);
        val = body->perform(this);
        if (val) break;
      }
    }
    else {
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        List* variable = 0;
        if ((*list)[i]->concrete_type() != Expression::LIST || variables.size() == 1) {
          variable = new (ctx.mem) List((*list)[i]->path(), (*list)[i]->position(), 1, List::COMMA);
          *variable << (*list)[i];
        }
        else {
          variable = static_cast<List*>((*list)[i]);
        }
        for (size_t j = 0, K = variables.size(); j < K; ++j) {
          if (j < variable->length()) {
            (*env)[variables[j]] = (*variable)[j];
          }
          else {
            (*env)[variables[j]] = new (ctx.mem) Null(expr->path(), expr->position());
          }
          val = body->perform(this);
          if (val) break;
        }
        if (val) break;
      }
    }
    env = new_env.parent();
    return val;
  }

  Expression* Eval::operator()(While* w)
  {
    Expression* pred = w->predicate();
    Block* body = w->block();
    while (*pred->perform(this)) {
      Expression* val = body->perform(this);
      if (val) return val;
    }
    return 0;
  }

  Expression* Eval::operator()(Return* r)
  {
    return r->value()->perform(this);
  }

  Expression* Eval::operator()(Warning* w)
  {
    Expression* message = w->message()->perform(this);
    To_String to_string;

    // try to use generic function
    if (env->has("@warn[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@warn[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_C_Function c_func = def->c_function();

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      Sass_Value* c_val = c_func(c_args, def->cookie());
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string result(unquote(message->perform(&to_string)));
    Backtrace top(backtrace, w->path(), w->position(), "");
    cerr << "WARNING: " << result;
    cerr << top.to_string(true);
    cerr << endl << endl;
    return 0;
  }

  Expression* Eval::operator()(Error* e)
  {
    Expression* message = e->message()->perform(this);
    To_String to_string;

    // try to use generic function
    if (env->has("@error[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@error[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_C_Function c_func = def->c_function();

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      Sass_Value* c_val = c_func(c_args, def->cookie());
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string result(unquote(message->perform(&to_string)));
    Backtrace top(backtrace, e->path(), e->position(), "");
    cerr << "Error: " << result;
    cerr << top.to_string(true);
    cerr << endl << endl;
    return 0;
  }

  Expression* Eval::operator()(Debug* d)
  {
    Expression* message = d->value()->perform(this);
    To_String to_string;

    // try to use generic function
    if (env->has("@debug[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@debug[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_C_Function c_func = def->c_function();

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      Sass_Value* c_val = c_func(c_args, def->cookie());
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string cwd(ctx.get_cwd());
    string result(unquote(message->perform(&to_string)));
    string rel_path(Sass::File::resolve_relative_path(d->path(), cwd, cwd));
    cerr << rel_path << ":" << d->position().line << ":" << " DEBUG: " << result;
    cerr << endl;
    return 0;
  }

  Expression* Eval::operator()(List* l)
  {
    if (l->is_expanded()) return l;
    List* ll = new (ctx.mem) List(l->path(),
                                  l->position(),
                                  l->length(),
                                  l->separator(),
                                  l->is_arglist());
    for (size_t i = 0, L = l->length(); i < L; ++i) {
      *ll << (*l)[i]->perform(this);
    }
    ll->is_expanded(true);
    return ll;
  }

  Expression* Eval::operator()(Map* m)
  {
    if (m->is_expanded()) return m;
    Map* mm = new (ctx.mem) Map(m->path(),
                                  m->position(),
                                  m->length());
    for (auto key : m->keys()) {
      *mm << std::make_pair(key->perform(this), m->at(key)->perform(this));
    }
    mm->is_expanded(true);
    return mm;
  }

  // -- only need to define two comparisons, and the rest can be implemented in terms of them
  bool eq(Expression*, Expression*, Context&, Eval*);
  bool lt(Expression*, Expression*, Context&);
  // -- arithmetic on the combinations that matter
  Expression* op_numbers(Context&, Binary_Expression*, Expression*, Expression*);
  Expression* op_number_color(Context&, Binary_Expression::Type, Expression*, Expression*);
  Expression* op_color_number(Context&, Binary_Expression::Type, Expression*, Expression*);
  Expression* op_colors(Context&, Binary_Expression::Type, Expression*, Expression*);
  Expression* op_strings(Context&, Binary_Expression::Type, Expression*, Expression*);

  Expression* Eval::operator()(Binary_Expression* b)
  {
    Binary_Expression::Type op_type = b->type();
    // don't eval delayed expressions (the '/' when used as a separator)
    if (op_type == Binary_Expression::DIV && b->is_delayed()) return b;
    // the logical connectives need to short-circuit
    Expression* lhs = b->left()->perform(this);
    switch (op_type) {
      case Binary_Expression::AND:
        return *lhs ? b->right()->perform(this) : lhs;
        break;

      case Binary_Expression::OR:
        return *lhs ? lhs : b->right()->perform(this);
        break;

      default:
        break;
    }
    // not a logical connective, so go ahead and eval the rhs
    Expression* rhs = b->right()->perform(this);

    // see if it's a relational expression
    switch(op_type) {
      case Binary_Expression::EQ:  return new (ctx.mem) Boolean(b->path(), b->position(), eq(lhs, rhs, ctx));
      case Binary_Expression::NEQ: return new (ctx.mem) Boolean(b->path(), b->position(), !eq(lhs, rhs, ctx));
      case Binary_Expression::GT:  return new (ctx.mem) Boolean(b->path(), b->position(), !lt(lhs, rhs, ctx) && !eq(lhs, rhs, ctx));
      case Binary_Expression::GTE: return new (ctx.mem) Boolean(b->path(), b->position(), !lt(lhs, rhs, ctx));
      case Binary_Expression::LT:  return new (ctx.mem) Boolean(b->path(), b->position(), lt(lhs, rhs, ctx));
      case Binary_Expression::LTE: return new (ctx.mem) Boolean(b->path(), b->position(), lt(lhs, rhs, ctx) || eq(lhs, rhs, ctx));

      default:                     break;
    }

    Expression::Concrete_Type l_type = lhs->concrete_type();
    Expression::Concrete_Type r_type = rhs->concrete_type();

    if (l_type == Expression::NUMBER && r_type == Expression::NUMBER) {
      return op_numbers(ctx, b, lhs, rhs);
    }
    if (l_type == Expression::NUMBER && r_type == Expression::COLOR) {
      return op_number_color(ctx, op_type, lhs, rhs);
    }
    if (l_type == Expression::COLOR && r_type == Expression::NUMBER) {
      return op_color_number(ctx, op_type, lhs, rhs);
    }
    if (l_type == Expression::COLOR && r_type == Expression::COLOR) {
      return op_colors(ctx, op_type, lhs, rhs);
    }
    return op_strings(ctx, op_type, lhs, rhs);
  }

  Expression* Eval::operator()(Unary_Expression* u)
  {
    Expression* operand = u->operand()->perform(this);
    if (u->type() == Unary_Expression::NOT) {
      Boolean* result = new (ctx.mem) Boolean(u->path(), u->position(), (bool)*operand);
      result->value(!result->value());
      return result;
    }
    else if (operand->concrete_type() == Expression::NUMBER) {
      Number* result = new (ctx.mem) Number(*static_cast<Number*>(operand));
      result->value(u->type() == Unary_Expression::MINUS
                    ? -result->value()
                    :  result->value());
      return result;
    }
    else {
      To_String to_string;
      // Special cases: +/- variables which evaluate to null ouput just +/-,
      // but +/- null itself outputs the string
      if (operand->concrete_type() == Expression::NULL_VAL && typeid(*(u->operand())) == typeid(Variable)) {
        u->operand(new (ctx.mem) String_Constant(u->path(), u->position(), ""));
      }
      else u->operand(operand);
      String_Constant* result = new (ctx.mem) String_Constant(u->path(),
                                                              u->position(),
                                                              u->perform(&to_string));
      return result;
    }
    // unreachable
    return u;
  }

  Expression* Eval::operator()(Function_Call* c)
  {
    string full_name(c->name() + "[f]");
    Arguments* args = c->arguments();
    if (full_name != "if[f]") {
      args = static_cast<Arguments*>(args->perform(this));
    }

    // try to use generic function
    if (!env->has(full_name)) {
      if (env->has("*[f]")) {
        full_name = "*[f]";
      }
    }

    // if it doesn't exist, just pass it through as a literal
    if (!env->has(full_name)) {
      Function_Call* lit = new (ctx.mem) Function_Call(c->path(),
                                                       c->position(),
                                                       c->name(),
                                                       args);
      To_String to_string;
      return new (ctx.mem) String_Constant(c->path(),
                                           c->position(),
                                           lit->perform(&to_string));
    }

    Expression*     result = c;
    Definition*     def    = static_cast<Definition*>((*env)[full_name]);
    Block*          body   = def->block();
    Native_Function func   = def->native_function();
    Sass_C_Function c_func = def->c_function();

    if (full_name != "if[f]") {
      for (size_t i = 0, L = args->length(); i < L; ++i) {
        (*args)[i]->value((*args)[i]->value()->perform(this));
      }
    }

    Parameters* params = def->parameters();
    Env new_env;
    new_env.link(def->environment());
    // bind("function " + c->name(), params, args, ctx, &new_env, this);
    // Env* old_env = env;
    // env = &new_env;

    // Backtrace here(backtrace, c->path(), c->line(), ", in function `" + c->name() + "`");
    // backtrace = &here;

    // if it's user-defined, eval the body
    if (body) {

      bind("function " + c->name(), params, args, ctx, &new_env, this);
      Env* old_env = env;
      env = &new_env;

      Backtrace here(backtrace, c->path(), c->position(), ", in function `" + c->name() + "`");
      backtrace = &here;

      result = body->perform(this);
      if (!result) {
        error(string("function ") + c->name() + " did not return a value", c->path(), c->position());
      }
      backtrace = here.parent;
      env = old_env;
    }
    // if it's native, invoke the underlying CPP function
    else if (func) {

      bind("function " + c->name(), params, args, ctx, &new_env, this);
      Env* old_env = env;
      env = &new_env;

      Backtrace here(backtrace, c->path(), c->position(), ", in function `" + c->name() + "`");
      backtrace = &here;

      result = func(*env, *old_env, ctx, def->signature(), c->path(), c->position(), backtrace);

      backtrace = here.parent;
      env = old_env;
    }
    // else if it's a user-defined c function
    else if (c_func) {

      if (full_name == "*[f]") {
        String_Constant *str = new (ctx.mem) String_Constant(c->path(), c->position(), c->name());
        Arguments* new_args = new (ctx.mem) Arguments(c->path(), c->position());
        *new_args << new (ctx.mem) Argument(c->path(), c->position(), str);
        *new_args += args;
        args = new_args;
      }

      // populates env with default values for params
      bind("function " + c->name(), params, args, ctx, &new_env, this);
      Env* old_env = env;
      env = &new_env;

      Backtrace here(backtrace, c->path(), c->position(), ", in function `" + c->name() + "`");
      backtrace = &here;

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(env->current_frame().size(), SASS_COMMA);
      for(size_t i = 0; i < params[0].length(); i++) {
        string key = params[0][i]->name();
        AST_Node* node = env->current_frame().at(key);
        Expression* arg = static_cast<Expression*>(node);
        sass_list_set_value(c_args, i, arg->perform(&to_c));
      }
      Sass_Value* c_val = c_func(c_args, def->cookie());
      if (sass_value_get_tag(c_val) == SASS_ERROR) {
        error("error in C function " + c->name() + ": " + sass_error_get_message(c_val), c->path(), c->position(), backtrace);
      } else if (sass_value_get_tag(c_val) == SASS_WARNING) {
        error("warning in C function " + c->name() + ": " + sass_warning_get_message(c_val), c->path(), c->position(), backtrace);
      }
      result = cval_to_astnode(c_val, ctx, backtrace, c->path(), c->position());

      backtrace = here.parent;
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      env = old_env;
    }
    // else it's an overloaded native function; resolve it
    else if (def->is_overload_stub()) {
      size_t arity = args->length();
      stringstream ss;
      ss << full_name << arity;
      string resolved_name(ss.str());
      if (!env->has(resolved_name)) error("overloaded function `" + string(c->name()) + "` given wrong number of arguments", c->path(), c->position());
      Definition* resolved_def = static_cast<Definition*>((*env)[resolved_name]);
      params = resolved_def->parameters();
      Env newer_env;
      newer_env.link(resolved_def->environment());
      bind("function " + c->name(), params, args, ctx, &newer_env, this);
      Env* old_env = env;
      env = &newer_env;

      Backtrace here(backtrace, c->path(), c->position(), ", in function `" + c->name() + "`");
      backtrace = &here;

      result = resolved_def->native_function()(*env, *old_env, ctx, resolved_def->signature(), c->path(), c->position(), backtrace);

      backtrace = here.parent;
      env = old_env;
    }

    // backtrace = here.parent;
    // env = old_env;
    result->position(c->position());
    return result;
  }

  Expression* Eval::operator()(Function_Call_Schema* s)
  {
    Expression* evaluated_name = s->name()->perform(this);
    Expression* evaluated_args = s->arguments()->perform(this);
    String_Schema* ss = new (ctx.mem) String_Schema(s->path(), s->position(), 2);
    (*ss) << evaluated_name << evaluated_args;
    return ss->perform(this);
  }

  Expression* Eval::operator()(Variable* v)
  {
    To_String to_string;
    string name(v->name());
    Expression* value = 0;
    if (env->has(name)) value = static_cast<Expression*>((*env)[name]);
    else error("unbound variable " + v->name(), v->path(), v->position());
    // cerr << "name: " << v->name() << "; type: " << typeid(*value).name() << "; value: " << value->perform(&to_string) << endl;
    if (typeid(*value) == typeid(Argument)) value = static_cast<Argument*>(value)->value();

    // behave according to as ruby sass (add leading zero)
    if (value->concrete_type() == Expression::NUMBER) {
      Number* n = static_cast<Number*>(value);
      value = new (ctx.mem) Number(n->path(),
                                   n->position(),
                                   n->value(),
                                   n->unit(),
                                   true);
    }
    else if (value->concrete_type() == Expression::STRING) {
      String_Constant* s = static_cast<String_Constant*>(value);
      value = new (ctx.mem) String_Constant(s->path(),
                                            s->position(),
                                            s->value());
    }

    // cerr << "\ttype is now: " << typeid(*value).name() << endl << endl;
    return value;
  }

  Expression* Eval::operator()(Textual* t)
  {
    using Prelexer::number;
    Expression* result = 0;
    bool zero = !( t->value().substr(0, 1) == "." ||
                   t->value().substr(0, 2) == "-." );

    const string& text = t->value();
    size_t num_pos = text.find_first_not_of(" \n\r\t");
    if (num_pos == string::npos) num_pos = text.length();
    size_t unit_pos = text.find_first_not_of("-+0123456789.", num_pos);
    if (unit_pos == string::npos) unit_pos = text.length();
    const string& num = text.substr(num_pos, unit_pos - num_pos);

    switch (t->type())
    {
      case Textual::NUMBER:
        result = new (ctx.mem) Number(t->path(),
                                      t->position(),
                                      atof(num.c_str()),
                                      "",
                                      zero);
        break;
      case Textual::PERCENTAGE:
        result = new (ctx.mem) Number(t->path(),
                                      t->position(),
                                      atof(num.c_str()),
                                      "%",
                                      zero);
        break;
      case Textual::DIMENSION:
        result = new (ctx.mem) Number(t->path(),
                                      t->position(),
                                      atof(num.c_str()),
                                      Token(number(text.c_str())),
                                      zero);
        break;
      case Textual::HEX: {
        string hext(t->value().substr(1)); // chop off the '#'
        if (hext.length() == 6) {
          string r(hext.substr(0,2));
          string g(hext.substr(2,2));
          string b(hext.substr(4,2));
          result = new (ctx.mem) Color(t->path(),
                                       t->position(),
                                       static_cast<double>(strtol(r.c_str(), NULL, 16)),
                                       static_cast<double>(strtol(g.c_str(), NULL, 16)),
                                       static_cast<double>(strtol(b.c_str(), NULL, 16)),
                                       1, true,
                                       t->value());
        }
        else {
          result = new (ctx.mem) Color(t->path(),
                                       t->position(),
                                       static_cast<double>(strtol(string(2,hext[0]).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(string(2,hext[1]).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(string(2,hext[2]).c_str(), NULL, 16)),
                                       1, false,
                                       t->value());
        }
      } break;
    }
    return result;
  }

  Expression* Eval::operator()(Number* n)
  {
    // behave according to as ruby sass (add leading zero)
    return new (ctx.mem) Number(n->path(),
                                n->position(),
                                n->value(),
                                n->unit(),
                                true);
  }

  Expression* Eval::operator()(Boolean* b)
  {
    return b;
  }

  char is_quoted(string str)
  {
    size_t len = str.length();
    if (len < 2) return 0;
    if ((str[0] == '"' && str[len-1] == '"') || (str[0] == '\'' && str[len-1] == '\'')) {
      return str[0];
    }
    else {
      return 0;
    }
  }

  Expression* Eval::operator()(String_Schema* s)
  {
    string acc;
    ctx._skip_source_map_update = true;
    To_String to_string(&ctx);
    ctx._skip_source_map_update = false;
    for (size_t i = 0, L = s->length(); i < L; ++i) {
      string chunk((*s)[i]->perform(this)->perform(&to_string));
      if (((s->quote_mark() && is_quoted(chunk)) || !s->quote_mark()) && (*s)[i]->is_interpolant()) { // some redundancy in that test
        acc += unquote(chunk);
      }
      else {
        acc += chunk;
      }
    }
    return new (ctx.mem) String_Constant(s->path(),
                                         s->position(),
                                         acc);
  }

  Expression* Eval::operator()(String_Constant* s)
  {
    if (!s->is_delayed() && ctx.names_to_colors.count(s->value())) {
      Color* c = new (ctx.mem) Color(*ctx.names_to_colors[s->value()]);
      c->path(s->path());
      c->position(s->position());
      c->disp(s->value());
      return c;
    }
    return s;
  }

  Expression* Eval::operator()(Feature_Query* q)
  {
    Feature_Query* qq = new (ctx.mem) Feature_Query(q->path(),
                                                    q->position(),
                                                    q->length());
    for (size_t i = 0, L = q->length(); i < L; ++i) {
      *qq << static_cast<Feature_Query_Condition*>((*q)[i]->perform(this));
    }
    return qq;
  }

  Expression* Eval::operator()(Feature_Query_Condition* c)
  {
    String* feature = c->feature();
    Expression* value = c->value();
    value = (value ? value->perform(this) : 0);
    Feature_Query_Condition* cc = new (ctx.mem) Feature_Query_Condition(c->path(),
                                                 c->position(),
                                                 c->length(),
                                                 feature,
                                                 value,
                                                 c->operand(),
                                                 c->is_root());
    for (size_t i = 0, L = c->length(); i < L; ++i) {
      *cc << static_cast<Feature_Query_Condition*>((*c)[i]->perform(this));
    }
    return cc;
  }

  Expression* Eval::operator()(Media_Query* q)
  {
    String* t = q->media_type();
    t = static_cast<String*>(t ? t->perform(this) : 0);
    Media_Query* qq = new (ctx.mem) Media_Query(q->path(),
                                                q->position(),
                                                t,
                                                q->length(),
                                                q->is_negated(),
                                                q->is_restricted());
    for (size_t i = 0, L = q->length(); i < L; ++i) {
      *qq << static_cast<Media_Query_Expression*>((*q)[i]->perform(this));
    }
    return qq;
  }

  Expression* Eval::operator()(Media_Query_Expression* e)
  {
    Expression* feature = e->feature();
    feature = (feature ? feature->perform(this) : 0);
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    return new (ctx.mem) Media_Query_Expression(e->path(),
                                                e->position(),
                                                feature,
                                                value,
                                                e->is_interpolated());
  }

  Expression* Eval::operator()(Null* n)
  {
    return n;
  }

  Expression* Eval::operator()(Argument* a)
  {
    Expression* val = a->value();
    val->is_delayed(false);
    val = val->perform(this);
    val->is_delayed(false);

    bool is_rest_argument = a->is_rest_argument();
    bool is_keyword_argument = a->is_keyword_argument();

    if (a->is_rest_argument()) {
      if (val->concrete_type() == Expression::MAP) {
        is_rest_argument = false;
        is_keyword_argument = true;
      }
      else
      if(val->concrete_type() != Expression::LIST) {
        List* wrapper = new (ctx.mem) List(val->path(),
                                           val->position(),
                                           0,
                                           List::COMMA,
                                           true);
        *wrapper << val;
        val = wrapper;
      }
    }
    return new (ctx.mem) Argument(a->path(),
                                  a->position(),
                                  val,
                                  a->name(),
                                  is_rest_argument,
                                  is_keyword_argument);
  }

  Expression* Eval::operator()(Arguments* a)
  {
    Arguments* aa = new (ctx.mem) Arguments(a->path(), a->position());
    for (size_t i = 0, L = a->length(); i < L; ++i) {
      *aa << static_cast<Argument*>((*a)[i]->perform(this));
    }
    return aa;
  }

  inline Expression* Eval::fallback_impl(AST_Node* n)
  {
    return static_cast<Expression*>(n);
  }

  // All the binary helpers.

  bool eq(Expression* lhs, Expression* rhs, Context& ctx)
  {
    Expression::Concrete_Type ltype = lhs->concrete_type();
    Expression::Concrete_Type rtype = rhs->concrete_type();
    if (ltype != rtype) return false;
    switch (ltype) {

      case Expression::BOOLEAN: {
        return static_cast<Boolean*>(lhs)->value() ==
               static_cast<Boolean*>(rhs)->value();
      } break;

      case Expression::NUMBER: {
        Number* l = static_cast<Number*>(lhs);
        Number* r = static_cast<Number*>(rhs);
        Number tmp_r(*r);
        tmp_r.normalize(l->find_convertible_unit());
        return l->unit() == tmp_r.unit() && l->value() == tmp_r.value()
               ? true
               : false;
      } break;

      case Expression::COLOR: {
        Color* l = static_cast<Color*>(lhs);
        Color* r = static_cast<Color*>(rhs);
        return l->r() == r->r() &&
               l->g() == r->g() &&
               l->b() == r->b() &&
               l->a() == r->a();
      } break;

      case Expression::STRING: {
        return unquote(static_cast<String_Constant*>(lhs)->value()) ==
               unquote(static_cast<String_Constant*>(rhs)->value());
      } break;

      case Expression::LIST: {
        List* l = static_cast<List*>(lhs);
        List* r = static_cast<List*>(rhs);
        if (l->length() != r->length()) return false;
        if (l->separator() != r->separator()) return false;
        for (size_t i = 0, L = l->length(); i < L; ++i) {
          if (!eq((*l)[i], (*r)[i], ctx)) return false;
        }
        return true;
      } break;

      case Expression::MAP: {
        Map* l = static_cast<Map*>(lhs);
        Map* r = static_cast<Map*>(rhs);
        if (l->length() != r->length()) return false;
        for (auto key : l->keys())
          if (!eq(l->at(key), r->at(key), ctx)) return false;
        return true;
      } break;
      case Expression::NULL_VAL: {
        return true;
      } break;

      default: break;
    }
    return false;
  }

  bool lt(Expression* lhs, Expression* rhs, Context& ctx)
  {
    if (lhs->concrete_type() != Expression::NUMBER ||
        rhs->concrete_type() != Expression::NUMBER)
      error("may only compare numbers", lhs->path(), lhs->position());
    Number* l = static_cast<Number*>(lhs);
    Number* r = static_cast<Number*>(rhs);
    Number tmp_r(*r);
    tmp_r.normalize(l->find_convertible_unit());
    string l_unit(l->unit());
    string r_unit(tmp_r.unit());
    if (!l_unit.empty() && !r_unit.empty() && l->unit() != tmp_r.unit()) {
      error("cannot compare numbers with incompatible units", l->path(), l->position());
    }
    return l->value() < tmp_r.value();
  }

  Expression* op_numbers(Context& ctx, Binary_Expression* b, Expression* lhs, Expression* rhs)
  {
    Number* l = static_cast<Number*>(lhs);
    Number* r = static_cast<Number*>(rhs);
    double lv = l->value();
    double rv = r->value();
    Binary_Expression::Type op = b->type();
    if (op == Binary_Expression::DIV && !rv) {
      return new (ctx.mem) String_Constant(l->path(), b->position(), "Infinity");
    }
    if (op == Binary_Expression::MOD && !rv) {
      error("division by zero", r->path(), r->position());
    }

    Number tmp(*r);
    tmp.normalize(l->find_convertible_unit());
    string l_unit(l->unit());
    string r_unit(tmp.unit());
    if (l_unit != r_unit && !l_unit.empty() && !r_unit.empty() &&
        (op == Binary_Expression::ADD || op == Binary_Expression::SUB)) {
      error("cannot add or subtract numbers with incompatible units", l->path(), l->position());
    }
    Number* v = new (ctx.mem) Number(*l);
    v->position(b->position());
    if (l_unit.empty() && (op == Binary_Expression::ADD || op == Binary_Expression::SUB || op == Binary_Expression::MOD)) {
      v->numerator_units() = r->numerator_units();
      v->denominator_units() = r->denominator_units();
    }

    v->value(ops[op](lv, tmp.value()));
    if (op == Binary_Expression::MUL) {
      for (size_t i = 0, S = r->numerator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r->numerator_units()[i]);
      }
      for (size_t i = 0, S = r->denominator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r->denominator_units()[i]);
      }
    }
    else if (op == Binary_Expression::DIV) {
      for (size_t i = 0, S = r->numerator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r->numerator_units()[i]);
      }
      for (size_t i = 0, S = r->denominator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r->denominator_units()[i]);
      }
    }
    v->normalize();
    return v;
  }

  Expression* op_number_color(Context& ctx, Binary_Expression::Type op, Expression* lhs, Expression* rhs)
  {
    Number* l = static_cast<Number*>(lhs);
    Color* r = static_cast<Color*>(rhs);
    // TODO: currently SASS converts colors to standard form when adding to strings;
    // when https://github.com/nex3/sass/issues/363 is added this can be removed to
    // preserve the original value
    r->disp("");
    double lv = l->value();
    switch (op) {
      case Binary_Expression::ADD:
      case Binary_Expression::MUL: {
        return new (ctx.mem) Color(l->path(),
                                   l->position(),
                                   ops[op](lv, r->r()),
                                   ops[op](lv, r->g()),
                                   ops[op](lv, r->b()),
                                   r->a());
      } break;
      case Binary_Expression::SUB:
      case Binary_Expression::DIV: {
        string sep(op == Binary_Expression::SUB ? "-" : "/");
        To_String to_string;
        string color(r->sixtuplet() ? r->perform(&to_string) :
                     Util::normalize_sixtuplet(r->perform(&to_string)));
        return new (ctx.mem) String_Constant(l->path(),
                                             l->position(),
                                             l->perform(&to_string)
                                             + sep
                                             + color);
      } break;
      case Binary_Expression::MOD: {
        error("cannot divide a number by a color", r->path(), r->position());
      } break;
      default: break; // caller should ensure that we don't get here
    }
    // unreachable
    return l;
  }

  Expression* op_color_number(Context& ctx, Binary_Expression::Type op, Expression* lhs, Expression* rhs)
  {
    Color* l = static_cast<Color*>(lhs);
    Number* r = static_cast<Number*>(rhs);
    double rv = r->value();
    if (op == Binary_Expression::DIV && !rv) error("division by zero", r->path(), r->position());
    return new (ctx.mem) Color(l->path(),
                               l->position(),
                               ops[op](l->r(), rv),
                               ops[op](l->g(), rv),
                               ops[op](l->b(), rv),
                               l->a());
  }

  Expression* op_colors(Context& ctx, Binary_Expression::Type op, Expression* lhs, Expression* rhs)
  {
    Color* l = static_cast<Color*>(lhs);
    Color* r = static_cast<Color*>(rhs);
    if (l->a() != r->a()) {
      error("alpha channels must be equal when combining colors", r->path(), r->position());
    }
    if ((op == Binary_Expression::DIV || op == Binary_Expression::MOD) &&
        (!r->r() || !r->g() ||!r->b())) {
      error("division by zero", r->path(), r->position());
    }
    return new (ctx.mem) Color(l->path(),
                               l->position(),
                               ops[op](l->r(), r->r()),
                               ops[op](l->g(), r->g()),
                               ops[op](l->b(), r->b()),
                               l->a());
  }

  Expression* op_strings(Context& ctx, Binary_Expression::Type op, Expression* lhs, Expression*rhs)
  {
    To_String to_string;
    Expression::Concrete_Type ltype = lhs->concrete_type();
    Expression::Concrete_Type rtype = rhs->concrete_type();

    string lstr(lhs->perform(&to_string));
    string rstr(rhs->perform(&to_string));

    bool l_str_quoted = ((Sass::String*)lhs) && ((Sass::String*)lhs)->needs_unquoting();
    bool r_str_quoted = ((Sass::String*)rhs) && ((Sass::String*)rhs)->needs_unquoting();
    bool l_str_color = ltype == Expression::STRING && ctx.names_to_colors.count(lstr) && !l_str_quoted;
    bool r_str_color = rtype == Expression::STRING && ctx.names_to_colors.count(rstr) && !r_str_quoted;

    bool unquoted = false;
    if (ltype == Expression::STRING && lstr[0] != '"' && lstr[0] != '\'') unquoted = true;
    if (l_str_color && r_str_color) {
      return op_colors(ctx, op, ctx.names_to_colors[lstr], ctx.names_to_colors[rstr]);
    }
    else if (l_str_color && rtype == Expression::COLOR) {
      return op_colors(ctx, op, ctx.names_to_colors[lstr], rhs);
    }
    else if (l_str_color && rtype == Expression::NUMBER) {
      return op_color_number(ctx, op, ctx.names_to_colors[lstr], rhs);
    }
    else if (ltype == Expression::COLOR && r_str_color) {
      return op_number_color(ctx, op, lhs, ctx.names_to_colors[rstr]);
    }
    else if (ltype == Expression::NUMBER && r_str_color) {
      return op_number_color(ctx, op, lhs, ctx.names_to_colors[rstr]);
    }
    if (op == Binary_Expression::MUL) error("invalid operands for multiplication", lhs->path(), lhs->position());
    if (op == Binary_Expression::MOD) error("invalid operands for modulo", lhs->path(), lhs->position());
    string sep;
    switch (op) {
      case Binary_Expression::SUB: sep = "-"; break;
      case Binary_Expression::DIV: sep = "/"; break;
      default:                         break;
    }
    if (ltype == Expression::NULL_VAL) error("invalid null operation: \"null plus "+quote(unquote(rstr), '"')+"\".", lhs->path(), lhs->position());
    if (rtype == Expression::NULL_VAL) error("invalid null operation: \""+quote(unquote(lstr), '"')+" plus null\".", lhs->path(), lhs->position());
    char q = '\0';
    if (lstr[0] == '"' || lstr[0] == '\'') q = lstr[0];
    else if (rstr[0] == '"' || rstr[0] == '\'') q = rstr[0];
    string result(unquote(lstr) + sep + unquote(rstr));
    return new (ctx.mem) String_Constant(lhs->path(),
                               lhs->position(),
                               unquoted ? result : quote(result, q));
  }

  Expression* cval_to_astnode(Sass_Value* v, Context& ctx, Backtrace* backtrace, string path, Position position)
  {
    using std::strlen;
    using std::strcpy;
    Expression* e = 0;
    switch (sass_value_get_tag(v)) {
      case SASS_BOOLEAN: {
        e = new (ctx.mem) Boolean(path, position, !!sass_boolean_get_value(v));
      } break;
      case SASS_NUMBER: {
        e = new (ctx.mem) Number(path, position, sass_number_get_value(v), sass_number_get_unit(v));
      } break;
      case SASS_COLOR: {
        e = new (ctx.mem) Color(path, position, sass_color_get_r(v), sass_color_get_g(v), sass_color_get_b(v), sass_color_get_a(v));
      } break;
      case SASS_STRING: {
        e = new (ctx.mem) String_Constant(path, position, sass_string_get_value(v));
      } break;
      case SASS_LIST: {
        List* l = new (ctx.mem) List(path, position, sass_list_get_length(v), sass_list_get_separator(v) == SASS_COMMA ? List::COMMA : List::SPACE);
        for (size_t i = 0, L = sass_list_get_length(v); i < L; ++i) {
          *l << cval_to_astnode(sass_list_get_value(v, i), ctx, backtrace, path, position);
        }
        e = l;
      } break;
      case SASS_MAP: {
        Map* m = new (ctx.mem) Map(path, position);
        for (size_t i = 0, L = sass_map_get_length(v); i < L; ++i) {
          *m << std::make_pair(
            cval_to_astnode(sass_map_get_key(v, i), ctx, backtrace, path, position),
            cval_to_astnode(sass_map_get_value(v, i), ctx, backtrace, path, position));
        }
        e = m;
      } break;
      case SASS_NULL: {
        e = new (ctx.mem) Null(path, position);
      } break;
      case SASS_ERROR: {
        error("Error in C function: " + string(sass_error_get_message(v)), path, position, backtrace);
      } break;
      case SASS_WARNING: {
        error("Warning in C function: " + string(sass_warning_get_message(v)), path, position, backtrace);
      } break;
    }
    return e;
  }

}
