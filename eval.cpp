#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <typeinfo>

#include "file.hpp"
#include "eval.hpp"
#include "ast.hpp"
#include "bind.hpp"
#include "util.hpp"
#include "to_string.hpp"
#include "inspect.hpp"
#include "environment.hpp"
#include "position.hpp"
#include "sass_values.h"
#include "to_c.hpp"
#include "context.hpp"
#include "backtrace.hpp"
#include "lexer.hpp"
#include "prelexer.hpp"
#include "parser.hpp"
#include "expand.hpp"
#include "color_maps.hpp"

namespace Sass {
  using namespace std;

  inline double add(double x, double y) { return x + y; }
  inline double sub(double x, double y) { return x - y; }
  inline double mul(double x, double y) { return x * y; }
  inline double div(double x, double y) { return x / y; } // x/0 checked by caller
  inline double mod(double x, double y) { return abs(fmod(x, y)); } // x/0 checked by caller
  typedef double (*bop)(double, double);
  bop ops[Sass_OP::NUM_OPS] = {
    0, 0, // and, or
    0, 0, 0, 0, 0, 0, // eq, neq, gt, gte, lt, lte
    add, sub, mul, div, mod
  };

  Eval::Eval(Expand& exp)
  : exp(exp),
    ctx(exp.ctx),
    listize(exp.ctx)
  { }
  Eval::~Eval() { }

  Context& Eval::context()
  {
    return ctx;
  }

  Env* Eval::environment()
  {
    return exp.environment();
  }

  Selector_List* Eval::selector()
  {
    return exp.selector();
  }

  Backtrace* Eval::backtrace()
  {
    return exp.backtrace();
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
    Env* env = exp.environment();
    string var(a->variable());
    if (a->is_global()) {
      if (a->is_default()) {
        if (env->has_global(var)) {
          Expression* e = dynamic_cast<Expression*>(env->get_global(var));
          if (!e || e->concrete_type() == Expression::NULL_VAL) {
            env->set_global(var, a->value()->perform(this));
          }
        }
        else {
          env->set_global(var, a->value()->perform(this));
        }
      }
      else {
        env->set_global(var, a->value()->perform(this));
      }
    }
    else if (a->is_default()) {
      if (env->has_lexical(var)) {
        auto cur = env;
        while (cur && cur->is_lexical()) {
          if (cur->has_local(var)) {
            if (AST_Node* node = cur->get_local(var)) {
              Expression* e = dynamic_cast<Expression*>(node);
              if (!e || e->concrete_type() == Expression::NULL_VAL) {
                cur->set_local(var, a->value()->perform(this));
              }
            }
            else {
              throw runtime_error("Env not in sync");
            }
            return 0;
          }
          cur = cur->parent();
        }
        throw runtime_error("Env not in sync");
      }
      else if (env->has_global(var)) {
        if (AST_Node* node = env->get_global(var)) {
          Expression* e = dynamic_cast<Expression*>(node);
          if (!e || e->concrete_type() == Expression::NULL_VAL) {
            env->set_global(var, a->value()->perform(this));
          }
        }
      }
      else if (env->is_lexical()) {
        env->set_local(var, a->value()->perform(this));
      }
      else {
        env->set_local(var, a->value()->perform(this));
      }
    }
    else {
      env->set_lexical(var, a->value()->perform(this));
    }
    return 0;
  }

  Expression* Eval::operator()(If* i)
  {
    Expression* rv = 0;
    Env env(exp.environment());
    exp.env_stack.push_back(&env);
    if (*i->predicate()->perform(this)) {
      rv = i->block()->perform(this);
    }
    else {
      Block* alt = i->alternative();
      if (alt) rv = alt->perform(this);
    }
    exp.env_stack.pop_back();
    return rv;
  }

  // For does not create a new env scope
  // But iteration vars are reset afterwards
  Expression* Eval::operator()(For* f)
  {
    string variable(f->variable());
    Expression* low = f->lower_bound()->perform(this);
    if (low->concrete_type() != Expression::NUMBER) {
      error("lower bound of `@for` directive must be numeric", low->pstate());
    }
    Expression* high = f->upper_bound()->perform(this);
    if (high->concrete_type() != Expression::NUMBER) {
      error("upper bound of `@for` directive must be numeric", high->pstate());
    }
    Number* sass_start = static_cast<Number*>(low);
    Number* sass_end = static_cast<Number*>(high);
    // check if units are valid for sequence
    if (sass_start->unit() != sass_end->unit()) {
      stringstream msg; msg << "Incompatible units: '"
        << sass_start->unit() << "' and '"
        << sass_end->unit() << "'.";
      error(msg.str(), low->pstate(), backtrace());
    }
    double start = sass_start->value();
    double end = sass_end->value();
    // only create iterator once in this environment
    Env* env = exp.environment();
    Number* it = new (env->mem) Number(low->pstate(), start, sass_end->unit());
    AST_Node* old_var = env->has_local(variable) ? env->get_local(variable) : 0;
    env->set_local(variable, it);
    Block* body = f->block();
    Expression* val = 0;
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start;
           i < end;
           ++i) {
        it->value(i);
        env->set_local(variable, it);
        val = body->perform(this);
        if (val) break;
      }
    } else {
      if (f->is_inclusive()) --end;
      for (double i = start;
           i > end;
           --i) {
        it->value(i);
        env->set_local(variable, it);
        val = body->perform(this);
        if (val) break;
      }
    }
    // restore original environment
    if (!old_var) env->del_local(variable);
    else env->set_local(variable, old_var);
    return val;
  }

  // Eval does not create a new env scope
  // But iteration vars are reset afterwards
  Expression* Eval::operator()(Each* e)
  {
    vector<string> variables(e->variables());
    Expression* expr = e->list()->perform(this);
    Env* env = exp.environment();
    List* list = 0;
    Map* map = 0;
    if (expr->concrete_type() == Expression::MAP) {
      map = static_cast<Map*>(expr);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = new (ctx.mem) List(expr->pstate(), 1, SASS_COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    // remember variables and then reset them
    vector<AST_Node*> old_vars(variables.size());
    for (size_t i = 0, L = variables.size(); i < L; ++i) {
      old_vars[i] = env->has_local(variables[i]) ? env->get_local(variables[i]) : 0;
      env->set_local(variables[i], 0);
    }
    Block* body = e->block();
    Expression* val = 0;

    if (map) {
      for (auto key : map->keys()) {
        Expression* value = map->at(key);

        if (variables.size() == 1) {
          List* variable = new (ctx.mem) List(map->pstate(), 2, SASS_SPACE);
          *variable << key;
          *variable << value;
          env->set_local(variables[0], variable);
        } else {
          env->set_local(variables[0], key);
          env->set_local(variables[1], value);
        }

        val = body->perform(this);
        if (val) break;
      }
    }
    else {
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        List* variable = 0;
        if ((*list)[i]->concrete_type() != Expression::LIST || variables.size() == 1) {
          variable = new (ctx.mem) List((*list)[i]->pstate(), 1, SASS_COMMA);
          *variable << (*list)[i];
        }
        else {
          variable = static_cast<List*>((*list)[i]);
        }
        for (size_t j = 0, K = variables.size(); j < K; ++j) {
          if (j < variable->length()) {
            env->set_local(variables[j], (*variable)[j]);
          }
          else {
            env->set_local(variables[j], new (ctx.mem) Null(expr->pstate()));
          }
          val = body->perform(this);
          if (val) break;
        }
        if (val) break;
      }
    }
    // restore original environment
    for (size_t j = 0, K = variables.size(); j < K; ++j) {
      if(!old_vars[j]) env->del_local(variables[j]);
      else env->set_local(variables[j], old_vars[j]);
    }
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
    To_String to_string(&ctx);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@warn[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@warn[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_options);
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string result(unquote(message->perform(&to_string)));
    Backtrace top(backtrace(), w->pstate(), "");
    cerr << "WARNING: " << result;
    cerr << top.to_string(true);
    cerr << endl << endl;
    return 0;
  }

  Expression* Eval::operator()(Error* e)
  {
    Expression* message = e->message()->perform(this);
    To_String to_string(&ctx);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@error[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@error[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_options);
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string result(unquote(message->perform(&to_string)));
    error(result, e->pstate());
    return 0;
  }

  Expression* Eval::operator()(Debug* d)
  {
    Expression* message = d->value()->perform(this);
    To_String to_string(&ctx);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@debug[f]")) {

      Definition* def = static_cast<Definition*>((*env)["@debug[f]"]);
      // Block*          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_options);
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    string cwd(ctx.get_cwd());
    string result(unquote(message->perform(&to_string)));
    string rel_path(Sass::File::resolve_relative_path(d->pstate().path, cwd, cwd));
    cerr << rel_path << ":" << d->pstate().line << ":" << " DEBUG: " << result;
    cerr << endl;
    return 0;
  }

  Expression* Eval::operator()(List* l)
  {
    if (l->is_expanded()) return l;
    List* ll = new (ctx.mem) List(l->pstate(),
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

    // make sure we're not starting with duplicate keys.
    // the duplicate key state will have been set in the parser phase.
    if (m->has_duplicate_key()) {
      To_String to_string(&ctx);
      error("Duplicate key \"" + m->get_duplicate_key()->perform(&to_string) + "\" in map " + m->perform(&to_string) + ".", m->pstate());
    }

    Map* mm = new (ctx.mem) Map(m->pstate(),
                                  m->length());
    for (auto key : m->keys()) {
      *mm << std::make_pair(key->perform(this), m->at(key)->perform(this));;
    }

    // check the evaluated keys aren't duplicates.
    if (mm->has_duplicate_key()) {
      To_String to_string(&ctx);
      error("Duplicate key \"" + mm->get_duplicate_key()->perform(&to_string) + "\" in map " + mm->perform(&to_string) + ".", mm->pstate());
    }

    mm->is_expanded(true);
    return mm;
  }

  Expression* Eval::operator()(Binary_Expression* b)
  {
    enum Sass_OP op_type = b->type();
    // don't eval delayed expressions (the '/' when used as a separator)
    if (op_type == Sass_OP::DIV && b->is_delayed()) return b;
    b->is_delayed(false);
    // if one of the operands is a '/' then make sure it's evaluated
    Expression* lhs = b->left()->perform(this);
    lhs->is_delayed(false);
    while (typeid(*lhs) == typeid(Binary_Expression)) lhs = lhs->perform(this);

    switch (op_type) {
      case Sass_OP::AND:
        return *lhs ? b->right()->perform(this) : lhs;
        break;

      case Sass_OP::OR:
        return *lhs ? lhs : b->right()->perform(this);
        break;

      default:
        break;
    }
    // not a logical connective, so go ahead and eval the rhs
    Expression* rhs = b->right()->perform(this);
    // maybe fully evaluate structure
    if (op_type == Sass_OP::EQ ||
        op_type == Sass_OP::NEQ ||
        op_type == Sass_OP::GT ||
        op_type == Sass_OP::GTE ||
        op_type == Sass_OP::LT ||
        op_type == Sass_OP::LTE)
    {
      rhs->is_expanded(false);
      rhs->set_delayed(false);
      rhs = rhs->perform(this);
    }
    else
    {
      // rhs->set_delayed(false);
      // rhs = rhs->perform(this);
    }

    // upgrade string to number if possible (issue #948)
    if (op_type == Sass_OP::DIV || op_type == Sass_OP::MUL) {
      if (String_Constant* str = dynamic_cast<String_Constant*>(rhs)) {
        string value(str->value());
        const char* start = value.c_str();
        if (Prelexer::sequence < Prelexer::number >(start) != 0) {
          rhs = new (ctx.mem) Textual(rhs->pstate(), Textual::DIMENSION, str->value());
          rhs->is_delayed(false); rhs = rhs->perform(this);
        }
      }
    }

    // see if it's a relational expression
    switch(op_type) {
      case Sass_OP::EQ:  return new (ctx.mem) Boolean(b->pstate(), eq(lhs, rhs, ctx));
      case Sass_OP::NEQ: return new (ctx.mem) Boolean(b->pstate(), !eq(lhs, rhs, ctx));
      case Sass_OP::GT:  return new (ctx.mem) Boolean(b->pstate(), !lt(lhs, rhs, ctx) && !eq(lhs, rhs, ctx));
      case Sass_OP::GTE: return new (ctx.mem) Boolean(b->pstate(), !lt(lhs, rhs, ctx));
      case Sass_OP::LT:  return new (ctx.mem) Boolean(b->pstate(), lt(lhs, rhs, ctx));
      case Sass_OP::LTE: return new (ctx.mem) Boolean(b->pstate(), lt(lhs, rhs, ctx) || eq(lhs, rhs, ctx));

      default:                     break;
    }

    Expression::Concrete_Type l_type = lhs->concrete_type();
    Expression::Concrete_Type r_type = rhs->concrete_type();

    if (l_type == Expression::NUMBER && r_type == Expression::NUMBER) {
      Number* l_n = dynamic_cast<Number*>(lhs);
      Number* r_n = dynamic_cast<Number*>(rhs);
      return op_numbers(ctx, op_type, l_n, r_n);
    }
    if (l_type == Expression::NUMBER && r_type == Expression::COLOR) {
      Number* l_n = dynamic_cast<Number*>(lhs);
      Color* r_c = dynamic_cast<Color*>(rhs);
      return op_number_color(ctx, op_type, l_n, r_c);
    }
    if (l_type == Expression::COLOR && r_type == Expression::NUMBER) {
      Color* l_c = dynamic_cast<Color*>(lhs);
      Number* r_n = dynamic_cast<Number*>(rhs);
      return op_color_number(ctx, op_type, l_c, r_n);
    }
    if (l_type == Expression::COLOR && r_type == Expression::COLOR) {
      Color* l_c = dynamic_cast<Color*>(lhs);
      Color* r_c = dynamic_cast<Color*>(rhs);
      return op_colors(ctx, op_type, l_c, r_c);
    }

    Expression* ex = op_strings(ctx, op_type, lhs, rhs);
    if (String_Constant* str = dynamic_cast<String_Constant*>(ex))
    {
      if (str->concrete_type() != Expression::STRING) return ex;
      String_Constant* lstr = dynamic_cast<String_Constant*>(lhs);
      String_Constant* rstr = dynamic_cast<String_Constant*>(rhs);
      if (String_Constant* org = lstr ? lstr : rstr)
      { str->quote_mark(org->quote_mark()); }
    }
    return ex;

  }

  Expression* Eval::operator()(Unary_Expression* u)
  {
    Expression* operand = u->operand()->perform(this);
    if (u->type() == Unary_Expression::NOT) {
      Boolean* result = new (ctx.mem) Boolean(u->pstate(), (bool)*operand);
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
      To_String to_string(&ctx);
      // Special cases: +/- variables which evaluate to null ouput just +/-,
      // but +/- null itself outputs the string
      if (operand->concrete_type() == Expression::NULL_VAL && typeid(*(u->operand())) == typeid(Variable)) {
        u->operand(new (ctx.mem) String_Quoted(u->pstate(), ""));
      }
      else u->operand(operand);
      String_Constant* result = new (ctx.mem) String_Quoted(u->pstate(),
                                                              u->perform(&to_string));
      return result;
    }
    // unreachable
    return u;
  }

  Expression* Eval::operator()(Function_Call* c)
  {
    if (backtrace()->parent != NULL && backtrace()->depth() > Constants::MaxCallStack) {
        ostringstream stm;
        stm << "Stack depth exceeded max of " << Constants::MaxCallStack;
        error(stm.str(), c->pstate(), backtrace());
    }
    string name(Util::normalize_underscores(c->name()));
    string full_name(name + "[f]");
    Arguments* args = c->arguments();
    if (full_name != "if[f]") {
      args = static_cast<Arguments*>(args->perform(this));
    }

    Env* env = environment();
    if (!env->has(full_name)) {
      if (!env->has("*[f]")) {
        // just pass it through as a literal
        Function_Call* lit = new (ctx.mem) Function_Call(c->pstate(),
                                                         c->name(),
                                                         args);
        To_String to_string(&ctx);
        return new (ctx.mem) String_Quoted(c->pstate(),
                                             lit->perform(&to_string));
      } else {
        // call generic function
        full_name = "*[f]";
      }
    }

    Definition* def = static_cast<Definition*>((*env)[full_name]);

    if (def->is_overload_stub()) {
      stringstream ss;
      ss << full_name
         << args->length();
      full_name = ss.str();
      string resolved_name(full_name);
      if (!env->has(resolved_name)) error("overloaded function `" + string(c->name()) + "` given wrong number of arguments", c->pstate());
      def = static_cast<Definition*>((*env)[resolved_name]);
    }

    Expression*     result = c;
    Block*          body   = def->block();
    Native_Function func   = def->native_function();
    Sass_Function_Entry c_function = def->c_function();

    Parameters* params = def->parameters();
    Env fn_env(def->environment());
    exp.env_stack.push_back(&fn_env);

    if (func || body) {
      bind("function " + c->name(), params, args, ctx, &fn_env, this);
      Backtrace here(backtrace(), c->pstate(), ", in function `" + c->name() + "`");
      exp.backtrace_stack.push_back(&here);
      // if it's user-defined, eval the body
      if (body) result = body->perform(this);
      // if it's native, invoke the underlying CPP function
      else result = func(fn_env, *env, ctx, def->signature(), c->pstate(), backtrace());
      if (!result) error(string("function ") + c->name() + " did not return a value", c->pstate());
      exp.backtrace_stack.pop_back();
    }

    // else if it's a user-defined c function
    // convert call into C-API compatible form
    else if (c_function) {
      Sass_Function_Fn c_func = sass_function_get_function(c_function);
      if (full_name == "*[f]") {
        String_Quoted *str = new (ctx.mem) String_Quoted(c->pstate(), c->name());
        Arguments* new_args = new (ctx.mem) Arguments(c->pstate());
        *new_args << new (ctx.mem) Argument(c->pstate(), str);
        *new_args += args;
        args = new_args;
      }

      // populates env with default values for params
      bind("function " + c->name(), params, args, ctx, &fn_env, this);

      Backtrace here(backtrace(), c->pstate(), ", in function `" + c->name() + "`");
      exp.backtrace_stack.push_back(&here);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(params[0].length(), SASS_COMMA);
      for(size_t i = 0; i < params[0].length(); i++) {
        string key = params[0][i]->name();
        AST_Node* node = fn_env.get_local(key);
        Expression* arg = static_cast<Expression*>(node);
        sass_list_set_value(c_args, i, arg->perform(&to_c));
      }
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_options);
      if (sass_value_get_tag(c_val) == SASS_ERROR) {
        error("error in C function " + c->name() + ": " + sass_error_get_message(c_val), c->pstate(), backtrace());
      } else if (sass_value_get_tag(c_val) == SASS_WARNING) {
        error("warning in C function " + c->name() + ": " + sass_warning_get_message(c_val), c->pstate(), backtrace());
      }
      result = cval_to_astnode(c_val, ctx, backtrace(), c->pstate());

      exp.backtrace_stack.pop_back();
      sass_delete_value(c_args);
      if (c_val != c_args)
        sass_delete_value(c_val);
    }

    // link back to function definition
    // only do this for custom functions
    if (result->pstate().file == string::npos)
      result->pstate(c->pstate());

    result->is_delayed(result->concrete_type() == Expression::STRING);
    if (!result->is_delayed()) result = result->perform(this);
    exp.env_stack.pop_back();
    return result;
  }

  Expression* Eval::operator()(Function_Call_Schema* s)
  {
    Expression* evaluated_name = s->name()->perform(this);
    Expression* evaluated_args = s->arguments()->perform(this);
    String_Schema* ss = new (ctx.mem) String_Schema(s->pstate(), 2);
    (*ss) << evaluated_name << evaluated_args;
    return ss->perform(this);
  }

  Expression* Eval::operator()(Variable* v)
  {
    To_String to_string(&ctx);
    string name(v->name());
    Expression* value = 0;
    Env* env = environment();
    if (env->has(name)) value = static_cast<Expression*>((*env)[name]);
    else error("Undefined variable: \"" + v->name() + "\".", v->pstate());
    // cerr << "name: " << v->name() << "; type: " << typeid(*value).name() << "; value: " << value->perform(&to_string) << endl;
    if (typeid(*value) == typeid(Argument)) value = static_cast<Argument*>(value)->value();

    // behave according to as ruby sass (add leading zero)
    if (value->concrete_type() == Expression::NUMBER) {
      value = new (ctx.mem) Number(*static_cast<Number*>(value));
      static_cast<Number*>(value)->zero(true);
    }
    else if (value->concrete_type() == Expression::STRING) {
      if (auto str = dynamic_cast<String_Quoted*>(value)) {
        value = new (ctx.mem) String_Quoted(*str);
      } else if (auto str = dynamic_cast<String_Constant*>(value)) {
        value = new (ctx.mem) String_Quoted(str->pstate(), str->perform(&to_string));
      }
    }
    else if (value->concrete_type() == Expression::LIST) {
      value = new (ctx.mem) List(*static_cast<List*>(value));
    }
    else if (value->concrete_type() == Expression::MAP) {
      value = new (ctx.mem) Map(*static_cast<Map*>(value));
    }
    else if (value->concrete_type() == Expression::BOOLEAN) {
      value = new (ctx.mem) Boolean(*static_cast<Boolean*>(value));
    }
    else if (value->concrete_type() == Expression::COLOR) {
      value = new (ctx.mem) Color(*static_cast<Color*>(value));
    }
    else if (value->concrete_type() == Expression::NULL_VAL) {
      value = new (ctx.mem) Null(value->pstate());
    }
    else if (value->concrete_type() == Expression::SELECTOR) {
      value = value->perform(this)->perform(&listize);
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
        result = new (ctx.mem) Number(t->pstate(),
                                      sass_atof(num.c_str()),
                                      "",
                                      zero);
        break;
      case Textual::PERCENTAGE:
        result = new (ctx.mem) Number(t->pstate(),
                                      sass_atof(num.c_str()),
                                      "%",
                                      zero);
        break;
      case Textual::DIMENSION:
        result = new (ctx.mem) Number(t->pstate(),
                                      sass_atof(num.c_str()),
                                      Token(number(text.c_str())),
                                      zero);
        break;
      case Textual::HEX: {
        if (t->value().substr(0, 1) != "#") {
          result = new (ctx.mem) String_Quoted(t->pstate(), t->value());
          break;
        }
        string hext(t->value().substr(1)); // chop off the '#'
        if (hext.length() == 6) {
          string r(hext.substr(0,2));
          string g(hext.substr(2,2));
          string b(hext.substr(4,2));
          result = new (ctx.mem) Color(t->pstate(),
                                       static_cast<double>(strtol(r.c_str(), NULL, 16)),
                                       static_cast<double>(strtol(g.c_str(), NULL, 16)),
                                       static_cast<double>(strtol(b.c_str(), NULL, 16)),
                                       1, true,
                                       t->value());
        }
        else {
          result = new (ctx.mem) Color(t->pstate(),
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
    return n;
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

  string Eval::interpolation(Expression* s) {
    Env* env = environment();
    if (String_Quoted* str_quoted = dynamic_cast<String_Quoted*>(s)) {
      if (str_quoted->quote_mark()) {
        if (str_quoted->quote_mark() == '*' || str_quoted->is_delayed()) {
          return evacuate_escapes(str_quoted->value());
        } else {
          return string_escape(quote(str_quoted->value(), str_quoted->quote_mark()));
        }
      } else {
        return evacuate_escapes(str_quoted->value());
      }
    } else if (String_Constant* str_constant = dynamic_cast<String_Constant*>(s)) {
      return evacuate_escapes(str_constant->value());
    } else if (dynamic_cast<Parent_Selector*>(s)) {
      To_String to_string(&ctx);
      Expression* sel = s->perform(this);
      return evacuate_quotes(sel ? sel->perform(&to_string) : "");

    } else if (String_Schema* str_schema = dynamic_cast<String_Schema*>(s)) {
      // To_String to_string(&ctx);
      // return evacuate_quotes(str_schema->perform(&to_string));

      string res = "";
      for(auto i : str_schema->elements())
        res += (interpolation(i));
      //ToDo: do this in one step
      auto esc = evacuate_escapes(res);
      auto unq = unquote(esc);
      if (unq == esc) {
        return string_to_output(res);
      } else {
        return evacuate_quotes(unq);
      }
    } else if (List* list = dynamic_cast<List*>(s)) {
      string acc = ""; // ToDo: different output styles
      string sep = list->separator() == SASS_COMMA ? "," : " ";
      if (ctx.output_style != COMPRESSED && sep == ",") sep += " ";
      bool initial = false;
      for(auto item : list->elements()) {
        if (initial) acc += sep;
        acc += interpolation(item);
        initial = true;
      }
      return evacuate_quotes(acc);
    } else if (Variable* var = dynamic_cast<Variable*>(s)) {
      string name(var->name());
      if (!env->has(name)) error("Undefined variable: \"" + var->name() + "\".", var->pstate());
      Expression* value = static_cast<Expression*>((*env)[name]);
      return evacuate_quotes(interpolation(value));
    } else if (dynamic_cast<Binary_Expression*>(s)) {
      Expression* ex = s->perform(this);
      return evacuate_quotes(interpolation(ex));
    } else if (dynamic_cast<Function_Call*>(s)) {
      Expression* ex = s->perform(this);
      return evacuate_quotes(interpolation(ex));
    } else if (dynamic_cast<Unary_Expression*>(s)) {
      Expression* ex = s->perform(this);
      return evacuate_quotes(interpolation(ex));
    } else if (dynamic_cast<Map*>(s)) {
      To_String to_string(&ctx);
      string dbg(s->perform(&to_string));
      error(dbg + " isn't a valid CSS value.", s->pstate());
      return dbg;
    } else {
      To_String to_string(&ctx);
      return evacuate_quotes(s->perform(&to_string));
    }
  }

  Expression* Eval::operator()(String_Schema* s)
  {
    string acc;
    for (size_t i = 0, L = s->length(); i < L; ++i) {
      if ((*s)[i]) acc += interpolation((*s)[i]);
    }
    String_Quoted* str = new (ctx.mem) String_Quoted(s->pstate(), acc);
    if (!str->quote_mark()) {
      str->value(string_unescape(str->value()));
    } else if (str->quote_mark()) {
      str->quote_mark('*');
    }
    str->is_delayed(true);
    return str;
  }

  Expression* Eval::operator()(String_Constant* s)
  {
    if (!s->is_delayed() && names_to_colors.count(s->value())) {
      Color* c = new (ctx.mem) Color(*name_to_color(s->value()));
      c->pstate(s->pstate());
      c->disp(s->value());
      return c;
    }
    return s;
  }

  Expression* Eval::operator()(String_Quoted* s)
  {
    return s;
  }

  Expression* Eval::operator()(Supports_Query* q)
  {
    Supports_Query* qq = new (ctx.mem) Supports_Query(q->pstate(),
                                                    q->length());
    for (size_t i = 0, L = q->length(); i < L; ++i) {
      *qq << static_cast<Supports_Condition*>((*q)[i]->perform(this));
    }
    return qq;
  }

  Expression* Eval::operator()(Supports_Condition* c)
  {
    String* feature = c->feature();
    Expression* value = c->value();
    value = (value ? value->perform(this) : 0);
    Supports_Condition* cc = new (ctx.mem) Supports_Condition(c->pstate(),
                                                 c->length(),
                                                 feature,
                                                 value,
                                                 c->operand(),
                                                 c->is_root());
    for (size_t i = 0, L = c->length(); i < L; ++i) {
      *cc << static_cast<Supports_Condition*>((*c)[i]->perform(this));
    }
    return cc;
  }

  Expression* Eval::operator()(At_Root_Expression* e)
  {
    Expression* feature = e->feature();
    feature = (feature ? feature->perform(this) : 0);
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    Expression* ee = new (ctx.mem) At_Root_Expression(e->pstate(),
                                                      static_cast<String*>(feature),
                                                      value,
                                                      e->is_interpolated());
    return ee;
  }

  Expression* Eval::operator()(Media_Query* q)
  {
    To_String to_string(&ctx);
    String* t = q->media_type();
    t = static_cast<String*>(t ? t->perform(this) : 0);
    Media_Query* qq = new (ctx.mem) Media_Query(q->pstate(),
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
    if (feature && dynamic_cast<String_Quoted*>(feature)) {
      feature = new (ctx.mem) String_Quoted(feature->pstate(),
                                              dynamic_cast<String_Quoted*>(feature)->value());
    }
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    if (value && dynamic_cast<String_Quoted*>(value)) {
      value = new (ctx.mem) String_Quoted(value->pstate(),
                                            dynamic_cast<String_Quoted*>(value)->value());
    }
    return new (ctx.mem) Media_Query_Expression(e->pstate(),
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
      else if(val->concrete_type() != Expression::LIST) {
        List* wrapper = new (ctx.mem) List(val->pstate(),
                                           0,
                                           SASS_COMMA,
                                           true);
        *wrapper << val;
        val = wrapper;
      }
    }
    return new (ctx.mem) Argument(a->pstate(),
                                  val,
                                  a->name(),
                                  is_rest_argument,
                                  is_keyword_argument);
  }

  Expression* Eval::operator()(Arguments* a)
  {
    Arguments* aa = new (ctx.mem) Arguments(a->pstate());
    for (size_t i = 0, L = a->length(); i < L; ++i) {
      *aa << static_cast<Argument*>((*a)[i]->perform(this));
    }
    return aa;
  }

  Expression* Eval::operator()(Comment* c)
  {
    return 0;
  }

  inline Expression* Eval::fallback_impl(AST_Node* n)
  {
    return static_cast<Expression*>(n);
  }

  // All the binary helpers.

  bool Eval::eq(Expression* lhs, Expression* rhs, Context& ctx)
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
        return (l->value() == r->value()) &&
               (l->numerator_units() == r->numerator_units()) &&
               (l->denominator_units() == r->denominator_units());
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
        string slhs = static_cast<String_Quoted*>(lhs)->value();
        string srhs = static_cast<String_Quoted*>(rhs)->value();
        return unquote(slhs) == unquote(srhs) &&
               (!(is_quoted(slhs) || is_quoted(srhs)) || slhs[0] == srhs[0]);
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

  bool Eval::lt(Expression* lhs, Expression* rhs, Context& ctx)
  {
    if (lhs->concrete_type() != Expression::NUMBER ||
        rhs->concrete_type() != Expression::NUMBER)
      error("may only compare numbers", lhs->pstate());
    Number* l = static_cast<Number*>(lhs);
    Number* r = static_cast<Number*>(rhs);
    Number tmp_r(*r);
    tmp_r.normalize(l->find_convertible_unit());
    string l_unit(l->unit());
    string r_unit(tmp_r.unit());
    if (!l_unit.empty() && !r_unit.empty() && l->unit() != tmp_r.unit()) {
      error("cannot compare numbers with incompatible units", l->pstate());
    }
    return l->value() < tmp_r.value();
  }

  Expression* Eval::op_numbers(Context& ctx, enum Sass_OP op, Number* l, Number* r)
  {
    double lv = l->value();
    double rv = r->value();
    if (op == Sass_OP::DIV && !rv) {
      return new (ctx.mem) String_Quoted(l->pstate(), "Infinity");
    }
    if (op == Sass_OP::MOD && !rv) {
      error("division by zero", r->pstate());
    }

    Number tmp(*r);
    tmp.normalize(l->find_convertible_unit());
    string l_unit(l->unit());
    string r_unit(tmp.unit());
    if (l_unit != r_unit && !l_unit.empty() && !r_unit.empty() &&
        (op == Sass_OP::ADD || op == Sass_OP::SUB)) {
      error("Incompatible units: '"+r_unit+"' and '"+l_unit+"'.", l->pstate());
    }
    Number* v = new (ctx.mem) Number(*l);
    v->pstate(l->pstate());
    if (l_unit.empty() && (op == Sass_OP::ADD || op == Sass_OP::SUB || op == Sass_OP::MOD)) {
      v->numerator_units() = r->numerator_units();
      v->denominator_units() = r->denominator_units();
    }

    if (op == Sass_OP::MUL) {
      v->value(ops[op](lv, rv));
      for (size_t i = 0, S = r->numerator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r->numerator_units()[i]);
      }
      for (size_t i = 0, S = r->denominator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r->denominator_units()[i]);
      }
    }
    else if (op == Sass_OP::DIV) {
      v->value(ops[op](lv, rv));
      for (size_t i = 0, S = r->numerator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r->numerator_units()[i]);
      }
      for (size_t i = 0, S = r->denominator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r->denominator_units()[i]);
      }
    } else {
      v->value(ops[op](lv, tmp.value()));
    }
    v->normalize();
    return v;
  }

  Expression* Eval::op_number_color(Context& ctx, enum Sass_OP op, Number* l, Color* r)
  {
    // TODO: currently SASS converts colors to standard form when adding to strings;
    // when https://github.com/nex3/sass/issues/363 is added this can be removed to
    // preserve the original value
    r->disp("");
    double lv = l->value();
    switch (op) {
      case Sass_OP::ADD:
      case Sass_OP::MUL: {
        return new (ctx.mem) Color(l->pstate(),
                                   ops[op](lv, r->r()),
                                   ops[op](lv, r->g()),
                                   ops[op](lv, r->b()),
                                   r->a());
      } break;
      case Sass_OP::SUB:
      case Sass_OP::DIV: {
        string sep(op == Sass_OP::SUB ? "-" : "/");
        To_String to_string(&ctx);
        string color(r->sixtuplet() && (ctx.output_style != COMPRESSED) ?
                     r->perform(&to_string) :
                     Util::normalize_sixtuplet(r->perform(&to_string)));
        return new (ctx.mem) String_Quoted(l->pstate(),
                                             l->perform(&to_string)
                                             + sep
                                             + color);
      } break;
      case Sass_OP::MOD: {
        error("cannot divide a number by a color", r->pstate());
      } break;
      default: break; // caller should ensure that we don't get here
    }
    // unreachable
    return l;
  }

  Expression* Eval::op_color_number(Context& ctx, enum Sass_OP op, Color* l, Number* r)
  {
    double rv = r->value();
    if (op == Sass_OP::DIV && !rv) error("division by zero", r->pstate());
    return new (ctx.mem) Color(l->pstate(),
                               ops[op](l->r(), rv),
                               ops[op](l->g(), rv),
                               ops[op](l->b(), rv),
                               l->a());
  }

  Expression* Eval::op_colors(Context& ctx, enum Sass_OP op, Color* l, Color* r)
  {
    if (l->a() != r->a()) {
      error("alpha channels must be equal when combining colors", r->pstate());
    }
    if ((op == Sass_OP::DIV || op == Sass_OP::MOD) &&
        (!r->r() || !r->g() ||!r->b())) {
      error("division by zero", r->pstate());
    }
    return new (ctx.mem) Color(l->pstate(),
                               ops[op](l->r(), r->r()),
                               ops[op](l->g(), r->g()),
                               ops[op](l->b(), r->b()),
                               l->a());
  }

  Expression* Eval::op_strings(Context& ctx, enum Sass_OP op, Expression* lhs, Expression*rhs)
  {
    To_String to_string(&ctx);
    Expression::Concrete_Type ltype = lhs->concrete_type();
    Expression::Concrete_Type rtype = rhs->concrete_type();

    string lstr(lhs->perform(&to_string));
    string rstr(rhs->perform(&to_string));

    bool l_str_quoted = ((Sass::String*)lhs) && ((Sass::String*)lhs)->sass_fix_1291();
    bool r_str_quoted = ((Sass::String*)rhs) && ((Sass::String*)rhs)->sass_fix_1291();
    bool l_str_color = ltype == Expression::STRING && names_to_colors.count(lstr) && !l_str_quoted;
    bool r_str_color = rtype == Expression::STRING && names_to_colors.count(rstr) && !r_str_quoted;

    if (l_str_color && r_str_color) {
      Color* l_c = name_to_color(lstr);
      Color* r_c = name_to_color(rstr);
      return op_colors(ctx, op, l_c, r_c);
    }
    else if (l_str_color && rtype == Expression::COLOR) {
      Color* l_c = name_to_color(lstr);
      Color* r_c = dynamic_cast<Color*>(rhs);
      return op_colors(ctx, op, l_c, r_c);
    }
    else if (ltype == Expression::COLOR && r_str_color) {
      Color* l_c = dynamic_cast<Color*>(lhs);
      Color* r_c = name_to_color(rstr);
      return op_colors(ctx, op, l_c, r_c);
    }
    else if (l_str_color && rtype == Expression::NUMBER) {
      Color* l_c = name_to_color(lstr);
      Number* r_n = dynamic_cast<Number*>(rhs);
      return op_color_number(ctx, op, l_c, r_n);
    }
    else if (ltype == Expression::NUMBER && r_str_color) {
      Number* l_n = dynamic_cast<Number*>(lhs);
      Color* r_c = name_to_color(rstr);
      return op_number_color(ctx, op, l_n, r_c);
    }
    if (op == Sass_OP::MUL) error("invalid operands for multiplication", lhs->pstate());
    if (op == Sass_OP::MOD) error("invalid operands for modulo", lhs->pstate());
    string sep;
    switch (op) {
      case Sass_OP::SUB: sep = "-"; break;
      case Sass_OP::DIV: sep = "/"; break;
      default:                         break;
    }
    if (ltype == Expression::NULL_VAL) error("invalid null operation: \"null plus "+quote(unquote(rstr), '"')+"\".", lhs->pstate());
    if (rtype == Expression::NULL_VAL) error("invalid null operation: \""+quote(unquote(lstr), '"')+" plus null\".", lhs->pstate());
    string result((lstr) + sep + (rstr));
    String_Quoted* str = new (ctx.mem) String_Quoted(lhs->pstate(), result);
    str->quote_mark(0);
    return str;
  }

  Expression* cval_to_astnode(Sass_Value* v, Context& ctx, Backtrace* backtrace, ParserState pstate)
  {
    using std::strlen;
    using std::strcpy;
    Expression* e = 0;
    switch (sass_value_get_tag(v)) {
      case SASS_BOOLEAN: {
        e = new (ctx.mem) Boolean(pstate, !!sass_boolean_get_value(v));
      } break;
      case SASS_NUMBER: {
        e = new (ctx.mem) Number(pstate, sass_number_get_value(v), sass_number_get_unit(v));
      } break;
      case SASS_COLOR: {
        e = new (ctx.mem) Color(pstate, sass_color_get_r(v), sass_color_get_g(v), sass_color_get_b(v), sass_color_get_a(v));
      } break;
      case SASS_STRING: {
        if (sass_string_is_quoted(v))
          e = new (ctx.mem) String_Quoted(pstate, sass_string_get_value(v));
        else {
          e = new (ctx.mem) String_Constant(pstate, sass_string_get_value(v));
        }
      } break;
      case SASS_LIST: {
        List* l = new (ctx.mem) List(pstate, sass_list_get_length(v), sass_list_get_separator(v));
        for (size_t i = 0, L = sass_list_get_length(v); i < L; ++i) {
          *l << cval_to_astnode(sass_list_get_value(v, i), ctx, backtrace, pstate);
        }
        e = l;
      } break;
      case SASS_MAP: {
        Map* m = new (ctx.mem) Map(pstate);
        for (size_t i = 0, L = sass_map_get_length(v); i < L; ++i) {
          *m << std::make_pair(
            cval_to_astnode(sass_map_get_key(v, i), ctx, backtrace, pstate),
            cval_to_astnode(sass_map_get_value(v, i), ctx, backtrace, pstate));
        }
        e = m;
      } break;
      case SASS_NULL: {
        e = new (ctx.mem) Null(pstate);
      } break;
      case SASS_ERROR: {
        error("Error in C function: " + string(sass_error_get_message(v)), pstate, backtrace);
      } break;
      case SASS_WARNING: {
        error("Warning in C function: " + string(sass_warning_get_message(v)), pstate, backtrace);
      } break;
    }
    return e;
  }

  Selector_List* Eval::operator()(Selector_List* s)
  {
    vector<Selector_List*> rv;
    Selector_List* sl = new (ctx.mem) Selector_List(s->pstate());
    for (size_t i = 0, iL = s->length(); i < iL; ++i) {
      rv.push_back(operator()((*s)[i]));
    }

    // we should actually permutate parent first
    // but here we have permutated the selector first
    size_t round = 0;
    while (round != string::npos) {
      bool abort = true;
      for (size_t i = 0, iL = rv.size(); i < iL; ++i) {
        if (rv[i]->length() > round) {
          *sl << (*rv[i])[round];
          abort = false;
        }
      }
      if (abort) {
        round = string::npos;
      } else {
        ++ round;
      }

    }
    return sl;
  }


  Selector_List* Eval::operator()(Complex_Selector* s)
  {
    if (s == 0) return 0;
    bool parentized = false;
    Complex_Selector* tail = s->tail();
    Compound_Selector* head = s->head();
    String* reference = s->reference();
    Complex_Selector::Combinator combinator = s->combinator();
    Selector_List* sl = new (ctx.mem) Selector_List(s->pstate());
    if (reference) reference = (String*) reference->perform(this);

    if (head) {
      // check if we have a parent selector reference (expands to list)
      if (head->length() > 1 && dynamic_cast<Parent_Selector*>((*head)[0])) {
        // do we have any parents to interpolate
        if (Selector_List* pr = selector()) {
          // parent will be prefixed
          Selector_List* ns = pr->cloneFully(ctx);
          // the tail can be re-attached unchanged
          for (size_t n = 0, nL = ns->length(); n < nL; ++n) {
            Complex_Selector* lst_t = (*ns)[n]->last();
            Compound_Selector* lst_h = lst_t->head();
            for (size_t i = 1, L = head->length(); i < L; ++i) *lst_h << (*head)[i];
            lst_t->tail(tail); // now connect old tail back to new intermediate
            lst_t->combinator(combinator); // and dont forget the combinator
            lst_t->reference(reference);
            // if (s->has_line_feed()) lst_t->has_line_feed(true); // and dont forget the combinator
          }
          return ns;
        }
        else {
          Complex_Selector* cpy = s->cloneFully(ctx);
          cpy->head(new (ctx.mem) Compound_Selector(head->pstate()));
          for (size_t i = 1, L = head->length(); i < L; ++i)
            *cpy->head() << (*head)[i];
          *sl << s;
          return sl;
        }
      }

      // have a simple
      if (head->length() == 1 && dynamic_cast<Parent_Selector*>((*head)[0])) {
        // do we have any parents to interpolate
        if (Selector_List* pr = selector()) {
          // parent will be prefixed
          Selector_List* ns = pr->cloneFully(ctx);
          // the tail can be re-attached unchanged
          for (size_t n = 0, nL = ns->length(); n < nL; ++n) {
            Complex_Selector* lst = (*ns)[n]->last();
            lst->tail(tail);
            if (combinator != Complex_Selector::ANCESTOR_OF) {
              if (lst->combinator()!= Complex_Selector::ANCESTOR_OF) {
                Complex_Selector* ins = s->clone(ctx);
                ins->head(0);
                ins->tail(tail);
                lst->tail(ins);
              } else {
                lst->combinator(combinator);
                lst->reference(reference);
              }
            }
            if (s->has_line_feed()) (*ns)[n]->has_line_feed(true);
            if (s->has_line_break()) lst->has_line_break(true);
          }
          return ns;
        }
        else {
          Complex_Selector* ss = s->cloneFully(ctx);
          // check if complex selector can be eliminated
          if (s->combinator() == Complex_Selector::ANCESTOR_OF)
          {
            if (s->has_line_feed()) tail->has_line_feed(true);
            if (s->has_line_break()) tail->has_line_break(true);
            *sl << tail;
          }
          else
          {
            *sl << ss;
          }
          return sl;
        }

      }

    }
    else
    {
      Selector_List* l = operator()(s->tail());
      for (size_t i = 0, L = l->length(); i < L; ++i) {
        Complex_Selector* ss = s->clone(ctx);
        ss->tail((*l)[i]);
        *sl << ss;
      }
      return sl;
    }

    if (parentized == false) {
      if (s->tail()) {
        Selector_List* tails = operator()(s->tail());
        for (size_t m = 0, mL = tails->length(); m < mL; ++m) {
          Complex_Selector* tailm = (*tails)[m];
          if(head && head->is_superselector_of(tailm)) {
            *sl << s;
          } else {
            Complex_Selector *ss = new(ctx.mem) Complex_Selector(*s);
            ss->tail(tailm);
            *sl << ss;
          }
        }
      }
      else {
        *sl << s;
      }
    }

    for (size_t i = 0, iL = sl->length(); i < iL; ++i) {

      if (!(*sl)[i]->head()) continue;
      if ((*sl)[i]->combinator() != Complex_Selector::ANCESTOR_OF) continue;
      if ((*sl)[i]->head()->is_empty_reference()) {
        // if ((*sl)[i]->has_line_feed()) {
          // if ((*sl)[i]->tail()) (*sl)[i]->tail()->has_line_feed(true);
        // }
        (*sl)[i] = (*sl)[i]->tail();
      }

    }

    return sl;
  }

  Attribute_Selector* Eval::operator()(Attribute_Selector* s)
  {
    String* attr = s->value();
    if (attr) { attr = static_cast<String*>(attr->perform(this)); }
    Attribute_Selector* ss = new (ctx.mem) Attribute_Selector(*s);
    ss->value(attr);
    return ss;
  }

  Selector_List* Eval::operator()(Selector_Schema* s)
  {
    To_String to_string;
    // the parser will look for a brace to end the selector
    string result_str(s->contents()->perform(this)->perform(&to_string) + "{");
    Parser p = Parser::from_c_str(result_str.c_str(), ctx, s->pstate());
    return operator()(p.parse_selector_list(exp.block_stack.back()->is_root()));
  }

  Expression* Eval::operator()(Parent_Selector* p)
  {
    Selector_List* pr = selector();
    exp.selector_stack.pop_back();
    if (pr) pr = operator()(pr);
    exp.selector_stack.push_back(pr);
    return pr;
  }

}
