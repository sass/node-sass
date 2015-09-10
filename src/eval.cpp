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
#include "sass/values.h"
#include "to_value.hpp"
#include "to_c.hpp"
#include "context.hpp"
#include "backtrace.hpp"
#include "lexer.hpp"
#include "prelexer.hpp"
#include "parser.hpp"
#include "expand.hpp"
#include "color_maps.hpp"

namespace Sass {

  inline double add(double x, double y) { return x + y; }
  inline double sub(double x, double y) { return x - y; }
  inline double mul(double x, double y) { return x * y; }
  inline double div(double x, double y) { return x / y; } // x/0 checked by caller
  inline double mod(double x, double y) { return std::abs(std::fmod(x, y)); } // x/0 checked by caller
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
    std::string var(a->variable());
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
              throw std::runtime_error("Env not in sync");
            }
            return 0;
          }
          cur = cur->parent();
        }
        throw std::runtime_error("Env not in sync");
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
    std::string variable(f->variable());
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
      std::stringstream msg; msg << "Incompatible units: '"
        << sass_start->unit() << "' and '"
        << sass_end->unit() << "'.";
      error(msg.str(), low->pstate(), backtrace());
    }
    double start = sass_start->value();
    double end = sass_end->value();
    // only create iterator once in this environment
    Env* env = exp.environment();
    Number* it = SASS_MEMORY_NEW(env->mem, Number, low->pstate(), start, sass_end->unit());
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
    std::vector<std::string> variables(e->variables());
    Expression* expr = e->list()->perform(this);
    Env* env = exp.environment();
    List* list = 0;
    Map* map = 0;
    if (expr->concrete_type() == Expression::MAP) {
      map = static_cast<Map*>(expr);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = SASS_MEMORY_NEW(ctx.mem, List, expr->pstate(), 1, SASS_COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    // remember variables and then reset them
    std::vector<AST_Node*> old_vars(variables.size());
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
          List* variable = SASS_MEMORY_NEW(ctx.mem, List, map->pstate(), 2, SASS_SPACE);
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
      bool arglist = list->is_arglist();
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        Expression* e = (*list)[i];
        // unwrap value if the expression is an argument
        if (Argument* arg = dynamic_cast<Argument*>(e)) e = arg->value();
        // check if we got passed a list of args (investigate)
        if (List* scalars = dynamic_cast<List*>(e)) {
          if (variables.size() == 1) {
            Expression* var = scalars;
            if (arglist) var = (*scalars)[0];
            env->set_local(variables[0], var);
          } else {
            for (size_t j = 0, K = variables.size(); j < K; ++j) {
              Expression* res = j >= scalars->length()
                ? SASS_MEMORY_NEW(ctx.mem, Null, expr->pstate())
                : (*scalars)[j];
              env->set_local(variables[j], res);
            }
          }
        } else {
          if (variables.size() > 0) {
            env->set_local(variables[0], e);
            for (size_t j = 1, K = variables.size(); j < K; ++j) {
              Expression* res = SASS_MEMORY_NEW(ctx.mem, Null, expr->pstate());
              env->set_local(variables[j], res);
            }
          }
        }
        val = body->perform(this);
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

    std::string result(unquote(message->perform(&to_string)));
    Backtrace top(backtrace(), w->pstate(), "");
    std::cerr << "WARNING: " << result;
    std::cerr << top.to_string(true);
    std::cerr << std::endl << std::endl;
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

    std::string result(unquote(message->perform(&to_string)));
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

    std::string cwd(ctx.get_cwd());
    std::string result(unquote(message->perform(&to_string)));
    std::string rel_path(Sass::File::resolve_relative_path(d->pstate().path, cwd, cwd));
    std::cerr << rel_path << ":" << d->pstate().line+1 << " DEBUG: " << result;
    std::cerr << std::endl;
    return 0;
  }

  Expression* Eval::operator()(List* l)
  {
    if (l->is_expanded()) return l;
    List* ll = SASS_MEMORY_NEW(ctx.mem, List,
                               l->pstate(),
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

    Map* mm = SASS_MEMORY_NEW(ctx.mem, Map,
                                m->pstate(),
                                m->length());
    for (auto key : m->keys()) {
      Expression* ex_key = key->perform(this);
      Expression* ex_val = m->at(key)->perform(this);
      *mm << std::make_pair(ex_key, ex_val);
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
        std::string value(str->value());
        const char* start = value.c_str();
        if (Prelexer::sequence < Prelexer::number >(start) != 0) {
          rhs = SASS_MEMORY_NEW(ctx.mem, Textual, rhs->pstate(), Textual::DIMENSION, str->value());
          rhs->is_delayed(false); rhs = rhs->perform(this);
        }
      }
    }

    // see if it's a relational expression
    switch(op_type) {
      case Sass_OP::EQ:  return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), eq(lhs, rhs));
      case Sass_OP::NEQ: return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), !eq(lhs, rhs));
      case Sass_OP::GT:  return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), !lt(lhs, rhs) && !eq(lhs, rhs));
      case Sass_OP::GTE: return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), !lt(lhs, rhs));
      case Sass_OP::LT:  return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), lt(lhs, rhs));
      case Sass_OP::LTE: return SASS_MEMORY_NEW(ctx.mem, Boolean, b->pstate(), lt(lhs, rhs) || eq(lhs, rhs));

      default:                     break;
    }

    Expression::Concrete_Type l_type = lhs->concrete_type();
    Expression::Concrete_Type r_type = rhs->concrete_type();

    int precision = (int)ctx.precision;
    bool compressed = ctx.output_style == COMPRESSED;
    if (l_type == Expression::NUMBER && r_type == Expression::NUMBER) {
      const Number* l_n = dynamic_cast<const Number*>(lhs);
      const Number* r_n = dynamic_cast<const Number*>(rhs);
      return op_numbers(ctx.mem, op_type, *l_n, *r_n, compressed, precision);
    }
    if (l_type == Expression::NUMBER && r_type == Expression::COLOR) {
      const Number* l_n = dynamic_cast<const Number*>(lhs);
      const Color* r_c = dynamic_cast<const Color*>(rhs);
      return op_number_color(ctx.mem, op_type, *l_n, *r_c, compressed, precision);
    }
    if (l_type == Expression::COLOR && r_type == Expression::NUMBER) {
      const Color* l_c = dynamic_cast<const Color*>(lhs);
      const Number* r_n = dynamic_cast<const Number*>(rhs);
      return op_color_number(ctx.mem, op_type, *l_c, *r_n, compressed, precision);
    }
    if (l_type == Expression::COLOR && r_type == Expression::COLOR) {
      const Color* l_c = dynamic_cast<const Color*>(lhs);
      const Color* r_c = dynamic_cast<const Color*>(rhs);
      return op_colors(ctx.mem, op_type, *l_c, *r_c, compressed, precision);
    }

    To_Value to_value(ctx, ctx.mem);
    Value* v_l = dynamic_cast<Value*>(lhs->perform(&to_value));
    Value* v_r = dynamic_cast<Value*>(rhs->perform(&to_value));
    Value* ex = op_strings(ctx.mem, op_type, *v_l, *v_r, compressed, precision);
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
      Boolean* result = SASS_MEMORY_NEW(ctx.mem, Boolean, u->pstate(), (bool)*operand);
      result->value(!result->value());
      return result;
    }
    else if (operand->concrete_type() == Expression::NUMBER) {
      Number* result = SASS_MEMORY_NEW(ctx.mem, Number, *static_cast<Number*>(operand));
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
        u->operand(SASS_MEMORY_NEW(ctx.mem, String_Quoted, u->pstate(), ""));
      }
      else u->operand(operand);
      String_Constant* result = SASS_MEMORY_NEW(ctx.mem, String_Quoted,
                                                  u->pstate(),
                                                  u->perform(&to_string));
      return result;
    }
    // unreachable
    return u;
  }

  Expression* Eval::operator()(Function_Call* c)
  {
    if (backtrace()->parent != NULL && backtrace()->depth() > Constants::MaxCallStack) {
        std::ostringstream stm;
        stm << "Stack depth exceeded max of " << Constants::MaxCallStack;
        error(stm.str(), c->pstate(), backtrace());
    }
    std::string name(Util::normalize_underscores(c->name()));
    std::string full_name(name + "[f]");
    Arguments* args = c->arguments();
    if (full_name != "if[f]") {
      args = static_cast<Arguments*>(args->perform(this));
    }

    Env* env = environment();
    if (!env->has(full_name)) {
      if (!env->has("*[f]")) {
        // just pass it through as a literal
        Function_Call* lit = SASS_MEMORY_NEW(ctx.mem, Function_Call,
                                             c->pstate(),
                                             c->name(),
                                             args);
        To_String to_string(&ctx);
        if (args->has_named_arguments()) {
          error("Function " + c->name() + " doesn't support keyword arguments", c->pstate());
        }
        return SASS_MEMORY_NEW(ctx.mem, String_Quoted,
                               c->pstate(),
                               lit->perform(&to_string));
      } else {
        // call generic function
        full_name = "*[f]";
      }
    }

    Definition* def = static_cast<Definition*>((*env)[full_name]);

    if (def->is_overload_stub()) {
      std::stringstream ss;
      ss << full_name
         << args->length();
      full_name = ss.str();
      std::string resolved_name(full_name);
      if (!env->has(resolved_name)) error("overloaded function `" + std::string(c->name()) + "` given wrong number of arguments", c->pstate());
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
      if (!result) error(std::string("function ") + c->name() + " did not return a value", c->pstate());
      exp.backtrace_stack.pop_back();
    }

    // else if it's a user-defined c function
    // convert call into C-API compatible form
    else if (c_function) {
      Sass_Function_Fn c_func = sass_function_get_function(c_function);
      if (full_name == "*[f]") {
        String_Quoted *str = SASS_MEMORY_NEW(ctx.mem, String_Quoted, c->pstate(), c->name());
        Arguments* new_args = SASS_MEMORY_NEW(ctx.mem, Arguments, c->pstate());
        *new_args << SASS_MEMORY_NEW(ctx.mem, Argument, c->pstate(), str);
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
        std::string key = params[0][i]->name();
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
      result = cval_to_astnode(ctx.mem, c_val, ctx, backtrace(), c->pstate());

      exp.backtrace_stack.pop_back();
      sass_delete_value(c_args);
      if (c_val != c_args)
        sass_delete_value(c_val);
    }

    // link back to function definition
    // only do this for custom functions
    if (result->pstate().file == std::string::npos)
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
    String_Schema* ss = SASS_MEMORY_NEW(ctx.mem, String_Schema, s->pstate(), 2);
    (*ss) << evaluated_name << evaluated_args;
    return ss->perform(this);
  }

  Expression* Eval::operator()(Variable* v)
  {
    To_String to_string(&ctx);
    std::string name(v->name());
    Expression* value = 0;
    Env* env = environment();
    if (env->has(name)) value = static_cast<Expression*>((*env)[name]);
    else error("Undefined variable: \"" + v->name() + "\".", v->pstate());
    // std::cerr << "name: " << v->name() << "; type: " << typeid(*value).name() << "; value: " << value->perform(&to_string) << std::endl;
    if (typeid(*value) == typeid(Argument)) value = static_cast<Argument*>(value)->value();

    // behave according to as ruby sass (add leading zero)
    if (value->concrete_type() == Expression::NUMBER) {
      value = SASS_MEMORY_NEW(ctx.mem, Number, *static_cast<Number*>(value));
      static_cast<Number*>(value)->zero(true);
    }
    else if (value->concrete_type() == Expression::STRING) {
      if (auto str = dynamic_cast<String_Quoted*>(value)) {
        value = SASS_MEMORY_NEW(ctx.mem, String_Quoted, *str);
      } else if (auto str = dynamic_cast<String_Constant*>(value)) {
        value = SASS_MEMORY_NEW(ctx.mem, String_Quoted, str->pstate(), str->perform(&to_string));
      }
    }
    else if (value->concrete_type() == Expression::LIST) {
      value = SASS_MEMORY_NEW(ctx.mem, List, *static_cast<List*>(value));
    }
    else if (value->concrete_type() == Expression::MAP) {
      value = SASS_MEMORY_NEW(ctx.mem, Map, *static_cast<Map*>(value));
    }
    else if (value->concrete_type() == Expression::BOOLEAN) {
      value = SASS_MEMORY_NEW(ctx.mem, Boolean, *static_cast<Boolean*>(value));
    }
    else if (value->concrete_type() == Expression::COLOR) {
      value = SASS_MEMORY_NEW(ctx.mem, Color, *static_cast<Color*>(value));
    }
    else if (value->concrete_type() == Expression::NULL_VAL) {
      value = SASS_MEMORY_NEW(ctx.mem, Null, value->pstate());
    }
    else if (value->concrete_type() == Expression::SELECTOR) {
      value = value->perform(this)->perform(&listize);
    }

    // std::cerr << "\ttype is now: " << typeid(*value).name() << std::endl << std::endl;
    return value;
  }

  Expression* Eval::operator()(Textual* t)
  {
    using Prelexer::number;
    Expression* result = 0;
    bool zero = !( t->value().substr(0, 1) == "." ||
                   t->value().substr(0, 2) == "-." );

    const std::string& text = t->value();
    size_t num_pos = text.find_first_not_of(" \n\r\t");
    if (num_pos == std::string::npos) num_pos = text.length();
    size_t unit_pos = text.find_first_not_of("-+0123456789.", num_pos);
    if (unit_pos == std::string::npos) unit_pos = text.length();
    const std::string& num = text.substr(num_pos, unit_pos - num_pos);

    switch (t->type())
    {
      case Textual::NUMBER:
        result = SASS_MEMORY_NEW(ctx.mem, Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 "",
                                 zero);
        break;
      case Textual::PERCENTAGE:
        result = SASS_MEMORY_NEW(ctx.mem, Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 "%",
                                 zero);
        break;
      case Textual::DIMENSION:
        result = SASS_MEMORY_NEW(ctx.mem, Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 Token(number(text.c_str())),
                                 zero);
        break;
      case Textual::HEX: {
        if (t->value().substr(0, 1) != "#") {
          result = SASS_MEMORY_NEW(ctx.mem, String_Quoted, t->pstate(), t->value());
          break;
        }
        std::string hext(t->value().substr(1)); // chop off the '#'
        if (hext.length() == 6) {
          std::string r(hext.substr(0,2));
          std::string g(hext.substr(2,2));
          std::string b(hext.substr(4,2));
          result = SASS_MEMORY_NEW(ctx.mem, Color,
                                   t->pstate(),
                                   static_cast<double>(strtol(r.c_str(), NULL, 16)),
                                   static_cast<double>(strtol(g.c_str(), NULL, 16)),
                                   static_cast<double>(strtol(b.c_str(), NULL, 16)),
                                   1, true,
                                   t->value());
        }
        else {
          result = SASS_MEMORY_NEW(ctx.mem, Color,
                                   t->pstate(),
                                   static_cast<double>(strtol(std::string(2,hext[0]).c_str(), NULL, 16)),
                                   static_cast<double>(strtol(std::string(2,hext[1]).c_str(), NULL, 16)),
                                   static_cast<double>(strtol(std::string(2,hext[2]).c_str(), NULL, 16)),
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

  char is_quoted(std::string str)
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

  std::string Eval::interpolation(Expression* s, bool into_quotes) {
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
      if (into_quotes && !str_constant->is_interpolant()) return str_constant->value();
      return evacuate_escapes(str_constant->value());
    } else if (dynamic_cast<Parent_Selector*>(s)) {
      To_String to_string(&ctx);
      Expression* sel = s->perform(this);
      return evacuate_quotes(sel ? sel->perform(&to_string) : "");

    } else if (String_Schema* str_schema = dynamic_cast<String_Schema*>(s)) {
      // To_String to_string(&ctx);
      // return evacuate_quotes(str_schema->perform(&to_string));

      std::string res = "";
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
      std::string acc = ""; // ToDo: different output styles
      std::string sep = list->separator() == SASS_COMMA ? "," : " ";
      if (ctx.output_style != COMPRESSED && sep == ",") sep += " ";
      bool initial = false;
      for(auto item : list->elements()) {
        if (item->concrete_type() != Expression::NULL_VAL) {
          if (initial) acc += sep;
          acc += interpolation(item);
          initial = true;
        }
      }
      return evacuate_quotes(acc);
    } else if (Variable* var = dynamic_cast<Variable*>(s)) {
      std::string name(var->name());
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
      std::string dbg(s->perform(&to_string));
      error(dbg + " isn't a valid CSS value.", s->pstate());
      return dbg;
    } else {
      To_String to_string(&ctx);
      return evacuate_quotes(s->perform(&to_string));
    }
  }

  Expression* Eval::operator()(String_Schema* s)
  {
    std::string acc;
    bool into_quotes = false;
    size_t L = s->length();
    if (L > 1) {
      if (String_Constant* l = dynamic_cast<String_Constant*>((*s)[0])) {
        if (String_Constant* r = dynamic_cast<String_Constant*>((*s)[L - 1])) {
          if (l->value()[0] == '"' && r->value()[r->value().size() - 1] == '"') into_quotes = true;
          if (l->value()[0] == '\'' && r->value()[r->value().size() - 1] == '\'') into_quotes = true;
        }
      }
    }
    for (size_t i = 0; i < L; ++i) {
      // really a very special fix, but this is the logic I got from
      // analyzing the ruby sass behavior and it actually seems to work
      // https://github.com/sass/libsass/issues/1333
      if (i == 0 && L > 1 && dynamic_cast<Function_Call*>((*s)[i])) {
        Expression* ex = (*s)[i]->perform(this);
        if (auto sq = dynamic_cast<String_Quoted*>(ex)) {
          if (sq->is_delayed() && ! s->has_interpolants()) {
            acc += string_escape(quote(sq->value(), sq->quote_mark()));
          } else {
            acc += interpolation((*s)[i], into_quotes);
          }
        } else if (ex) {
          acc += interpolation((*s)[i], into_quotes);
        }
      } else if ((*s)[i]) {
        acc += interpolation((*s)[i], into_quotes);
      }
    }
    String_Quoted* str = SASS_MEMORY_NEW(ctx.mem, String_Quoted, s->pstate(), acc);
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
    if (!s->is_delayed() && name_to_color(s->value())) {
      Color* c = SASS_MEMORY_NEW(ctx.mem, Color, *name_to_color(s->value()));
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

  Expression* Eval::operator()(Supports_Operator* c)
  {
    Expression* left = c->left()->perform(this);
    Expression* right = c->right()->perform(this);
    Supports_Operator* cc = SASS_MEMORY_NEW(ctx.mem, Supports_Operator,
                                 c->pstate(),
                                 static_cast<Supports_Condition*>(left),
                                 static_cast<Supports_Condition*>(right),
                                 c->operand());
    return cc;
  }

  Expression* Eval::operator()(Supports_Negation* c)
  {
    Expression* condition = c->condition()->perform(this);
    Supports_Negation* cc = SASS_MEMORY_NEW(ctx.mem, Supports_Negation,
                                 c->pstate(),
                                 static_cast<Supports_Condition*>(condition));
    return cc;
  }

  Expression* Eval::operator()(Supports_Declaration* c)
  {
    Expression* feature = c->feature()->perform(this);
    Expression* value = c->value()->perform(this);
    Supports_Declaration* cc = SASS_MEMORY_NEW(ctx.mem, Supports_Declaration,
                              c->pstate(),
                              feature,
                              value);
    return cc;
  }

  Expression* Eval::operator()(Supports_Interpolation* c)
  {
    Expression* value = c->value()->perform(this);
    Supports_Interpolation* cc = SASS_MEMORY_NEW(ctx.mem, Supports_Interpolation,
                            c->pstate(),
                            value);
    return cc;
  }

  Expression* Eval::operator()(At_Root_Expression* e)
  {
    Expression* feature = e->feature();
    feature = (feature ? feature->perform(this) : 0);
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    Expression* ee = SASS_MEMORY_NEW(ctx.mem, At_Root_Expression,
                                     e->pstate(),
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
    Media_Query* qq = SASS_MEMORY_NEW(ctx.mem, Media_Query,
                                      q->pstate(),
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
      feature = SASS_MEMORY_NEW(ctx.mem, String_Quoted,
                                  feature->pstate(),
                                  dynamic_cast<String_Quoted*>(feature)->value());
    }
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    if (value && dynamic_cast<String_Quoted*>(value)) {
      value = SASS_MEMORY_NEW(ctx.mem, String_Quoted,
                                value->pstate(),
                                dynamic_cast<String_Quoted*>(value)->value());
    }
    return SASS_MEMORY_NEW(ctx.mem, Media_Query_Expression,
                           e->pstate(),
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
        List* wrapper = SASS_MEMORY_NEW(ctx.mem, List,
                                        val->pstate(),
                                        0,
                                        SASS_COMMA,
                                        true);
        *wrapper << val;
        val = wrapper;
      }
    }
    return SASS_MEMORY_NEW(ctx.mem, Argument,
                           a->pstate(),
                           val,
                           a->name(),
                           is_rest_argument,
                           is_keyword_argument);
  }

  Expression* Eval::operator()(Arguments* a)
  {
    Arguments* aa = SASS_MEMORY_NEW(ctx.mem, Arguments, a->pstate());
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

  bool Eval::eq(Expression* lhs, Expression* rhs)
  {
    // use compare operator from ast node
    return lhs && rhs && *lhs == *rhs;
  }

  bool Eval::lt(Expression* lhs, Expression* rhs)
  {
    Number* l = dynamic_cast<Number*>(lhs);
    Number* r = dynamic_cast<Number*>(rhs);
    if (!l) error("may only compare numbers", lhs->pstate());
    if (!r) error("may only compare numbers", rhs->pstate());
    // use compare operator from ast node
    return *l < *r;
  }

  Value* Eval::op_numbers(Memory_Manager& mem, enum Sass_OP op, const Number& l, const Number& r, bool compressed, int precision)
  {
    double lv = l.value();
    double rv = r.value();
    if (op == Sass_OP::DIV && !rv) {
      return SASS_MEMORY_NEW(mem, String_Quoted, l.pstate(), "Infinity");
    }
    if (op == Sass_OP::MOD && !rv) {
      error("division by zero", r.pstate());
    }

    Number tmp(r);
    bool strict = op != Sass_OP::MUL && op != Sass_OP::DIV;
    tmp.normalize(l.find_convertible_unit(), strict);
    std::string l_unit(l.unit());
    std::string r_unit(tmp.unit());
    if (l_unit != r_unit && !l_unit.empty() && !r_unit.empty() &&
        (op == Sass_OP::ADD || op == Sass_OP::SUB)) {
      error("Incompatible units: '"+r_unit+"' and '"+l_unit+"'.", l.pstate());
    }
    Number* v = SASS_MEMORY_NEW(mem, Number, l);
    v->pstate(l.pstate());
    if (l_unit.empty() && (op == Sass_OP::ADD || op == Sass_OP::SUB || op == Sass_OP::MOD)) {
      v->numerator_units() = r.numerator_units();
      v->denominator_units() = r.denominator_units();
    }

    if (op == Sass_OP::MUL) {
      v->value(ops[op](lv, rv));
      for (size_t i = 0, S = r.numerator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r.numerator_units()[i]);
      }
      for (size_t i = 0, S = r.denominator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r.denominator_units()[i]);
      }
    }
    else if (op == Sass_OP::DIV) {
      v->value(ops[op](lv, rv));
      for (size_t i = 0, S = r.numerator_units().size(); i < S; ++i) {
        v->denominator_units().push_back(r.numerator_units()[i]);
      }
      for (size_t i = 0, S = r.denominator_units().size(); i < S; ++i) {
        v->numerator_units().push_back(r.denominator_units()[i]);
      }
    } else {
      v->value(ops[op](lv, tmp.value()));
    }
    v->normalize();
    return v;
  }

  Value* Eval::op_number_color(Memory_Manager& mem, enum Sass_OP op, const Number& l, const Color& rh, bool compressed, int precision)
  {
    Color r(rh);
    r.disp("");
    double lv = l.value();
    switch (op) {
      case Sass_OP::ADD:
      case Sass_OP::MUL: {
        return SASS_MEMORY_NEW(mem, Color,
                               l.pstate(),
                               ops[op](lv, r.r()),
                               ops[op](lv, r.g()),
                               ops[op](lv, r.b()),
                               r.a());
      } break;
      case Sass_OP::SUB:
      case Sass_OP::DIV: {
        std::string sep(op == Sass_OP::SUB ? "-" : "/");
        std::string color(r.to_string(compressed||!r.sixtuplet(), precision));
        return SASS_MEMORY_NEW(mem, String_Quoted,
                               l.pstate(),
                               l.to_string(compressed, precision)
                               + sep
                               + color);
      } break;
      case Sass_OP::MOD: {
        error("cannot divide a number by a color", r.pstate());
      } break;
      default: break; // caller should ensure that we don't get here
    }
    // unreachable
    return SASS_MEMORY_NEW(mem, Color, rh);
  }

  Value* Eval::op_color_number(Memory_Manager& mem, enum Sass_OP op, const Color& l, const Number& r, bool compressed, int precision)
  {
    double rv = r.value();
    if (op == Sass_OP::DIV && !rv) error("division by zero", r.pstate());
    return SASS_MEMORY_NEW(mem, Color,
                           l.pstate(),
                           ops[op](l.r(), rv),
                           ops[op](l.g(), rv),
                           ops[op](l.b(), rv),
                           l.a());
  }

  Value* Eval::op_colors(Memory_Manager& mem, enum Sass_OP op, const Color& l, const Color& r, bool compressed, int precision)
  {
    if (l.a() != r.a()) {
      error("alpha channels must be equal when combining colors", r.pstate());
    }
    if (op == Sass_OP::DIV && (!r.r() || !r.g() ||!r.b())) {
      error("division by zero", r.pstate());
    }
    return SASS_MEMORY_NEW(mem, Color,
                           l.pstate(),
                           ops[op](l.r(), r.r()),
                           ops[op](l.g(), r.g()),
                           ops[op](l.b(), r.b()),
                           l.a());
  }

  Value* Eval::op_strings(Memory_Manager& mem, enum Sass_OP op, Value& lhs, Value& rhs, bool compressed, int precision)
  {
    Expression::Concrete_Type ltype = lhs.concrete_type();
    Expression::Concrete_Type rtype = rhs.concrete_type();

    String_Quoted* lqstr = dynamic_cast<String_Quoted*>(&lhs);
    String_Quoted* rqstr = dynamic_cast<String_Quoted*>(&rhs);

    std::string lstr(lqstr ? lqstr->value() : lhs.to_string(compressed, precision));
    std::string rstr(rqstr ? rqstr->value() : rhs.to_string(compressed, precision));

    bool l_str_quoted = ((Sass::String*)&lhs) && ((Sass::String*)&lhs)->sass_fix_1291();
    bool r_str_quoted = ((Sass::String*)&rhs) && ((Sass::String*)&rhs)->sass_fix_1291();
    bool l_str_color = ltype == Expression::STRING && name_to_color(lstr) && !l_str_quoted;
    bool r_str_color = rtype == Expression::STRING && name_to_color(rstr) && !r_str_quoted;

    if (l_str_color && r_str_color) {
      const Color* c_l = name_to_color(lstr);
      const Color* c_r = name_to_color(rstr);
      return op_colors(mem, op,*c_l, *c_r, compressed, precision);
    }
    else if (l_str_color && rtype == Expression::COLOR) {
      const Color* c_l = name_to_color(lstr);
      const Color* c_r = dynamic_cast<const Color*>(&rhs);
      return op_colors(mem, op, *c_l, *c_r, compressed, precision);
    }
    else if (ltype == Expression::COLOR && r_str_color) {
      const Color* c_l = dynamic_cast<const Color*>(&lhs);
      const Color* c_r = name_to_color(rstr);
      return op_colors(mem, op, *c_l, *c_r, compressed, precision);
    }
    else if (l_str_color && rtype == Expression::NUMBER) {
      const Color* c_l = name_to_color(lstr);
      const Number* n_r = dynamic_cast<const Number*>(&rhs);
      return op_color_number(mem, op, *c_l, *n_r, compressed, precision);
    }
    else if (ltype == Expression::NUMBER && r_str_color) {
      const Number* n_l = dynamic_cast<const Number*>(&lhs);
      const Color* c_r = name_to_color(rstr);
      return op_number_color(mem, op, *n_l, *c_r, compressed, precision);
    }
    if (op == Sass_OP::MUL) error("invalid operands for multiplication", lhs.pstate());
    if (op == Sass_OP::MOD) error("invalid operands for modulo", lhs.pstate());
    std::string sep;
    switch (op) {
      case Sass_OP::SUB: sep = "-"; break;
      case Sass_OP::DIV: sep = "/"; break;
      default:                         break;
    }
    if (ltype == Expression::NULL_VAL) error("invalid null operation: \"null plus "+quote(unquote(rstr), '"')+"\".", lhs.pstate());
    if (rtype == Expression::NULL_VAL) error("invalid null operation: \""+quote(unquote(lstr), '"')+" plus null\".", rhs.pstate());

    if ( (ltype == Expression::STRING || sep == "") &&
         (sep != "/" || !rqstr || !rqstr->quote_mark())
    ) {
      char quote_mark = 0;
      std::string unq(unquote(lstr + sep + rstr, &quote_mark, true));
      if (quote_mark && quote_mark != '*') {
        return SASS_MEMORY_NEW(mem, String_Constant, lhs.pstate(), quote_mark + unq + quote_mark);
      }
      return SASS_MEMORY_NEW(mem, String_Quoted, lhs.pstate(), lstr + sep + rstr);
    }
    return SASS_MEMORY_NEW(mem, String_Constant, lhs.pstate(), (lstr) + sep + quote(rstr));
  }

  Expression* cval_to_astnode(Memory_Manager& mem, union Sass_Value* v, Context& ctx, Backtrace* backtrace, ParserState pstate)
  {
    using std::strlen;
    using std::strcpy;
    Expression* e = 0;
    switch (sass_value_get_tag(v)) {
      case SASS_BOOLEAN: {
        e = SASS_MEMORY_NEW(mem, Boolean, pstate, !!sass_boolean_get_value(v));
      } break;
      case SASS_NUMBER: {
        e = SASS_MEMORY_NEW(mem, Number, pstate, sass_number_get_value(v), sass_number_get_unit(v));
      } break;
      case SASS_COLOR: {
        e = SASS_MEMORY_NEW(mem, Color, pstate, sass_color_get_r(v), sass_color_get_g(v), sass_color_get_b(v), sass_color_get_a(v));
      } break;
      case SASS_STRING: {
        if (sass_string_is_quoted(v))
          e = SASS_MEMORY_NEW(mem, String_Quoted, pstate, sass_string_get_value(v));
        else {
          e = SASS_MEMORY_NEW(mem, String_Constant, pstate, sass_string_get_value(v));
        }
      } break;
      case SASS_LIST: {
        List* l = SASS_MEMORY_NEW(mem, List, pstate, sass_list_get_length(v), sass_list_get_separator(v));
        for (size_t i = 0, L = sass_list_get_length(v); i < L; ++i) {
          *l << cval_to_astnode(mem, sass_list_get_value(v, i), ctx, backtrace, pstate);
        }
        e = l;
      } break;
      case SASS_MAP: {
        Map* m = SASS_MEMORY_NEW(mem, Map, pstate);
        for (size_t i = 0, L = sass_map_get_length(v); i < L; ++i) {
          *m << std::make_pair(
            cval_to_astnode(mem, sass_map_get_key(v, i), ctx, backtrace, pstate),
            cval_to_astnode(mem, sass_map_get_value(v, i), ctx, backtrace, pstate));
        }
        e = m;
      } break;
      case SASS_NULL: {
        e = SASS_MEMORY_NEW(mem, Null, pstate);
      } break;
      case SASS_ERROR: {
        error("Error in C function: " + std::string(sass_error_get_message(v)), pstate, backtrace);
      } break;
      case SASS_WARNING: {
        error("Warning in C function: " + std::string(sass_warning_get_message(v)), pstate, backtrace);
      } break;
    }
    return e;
  }

  Selector_List* Eval::operator()(Selector_List* s)
  {
    std::vector<Selector_List*> rv;
    Selector_List* sl = SASS_MEMORY_NEW(ctx.mem, Selector_List, s->pstate());
    for (size_t i = 0, iL = s->length(); i < iL; ++i) {
      rv.push_back(operator()((*s)[i]));
    }

    // we should actually permutate parent first
    // but here we have permutated the selector first
    size_t round = 0;
    while (round != std::string::npos) {
      bool abort = true;
      for (size_t i = 0, iL = rv.size(); i < iL; ++i) {
        if (rv[i]->length() > round) {
          *sl << (*rv[i])[round];
          abort = false;
        }
      }
      if (abort) {
        round = std::string::npos;
      } else {
        ++ round;
      }

    }
    return sl;
  }


  Selector_List* Eval::operator()(Complex_Selector* s)
  {
    return s->parentize(selector(), ctx);

  }

  Attribute_Selector* Eval::operator()(Attribute_Selector* s)
  {
    String* attr = s->value();
    if (attr) { attr = static_cast<String*>(attr->perform(this)); }
    Attribute_Selector* ss = SASS_MEMORY_NEW(ctx.mem, Attribute_Selector, *s);
    ss->value(attr);
    return ss;
  }

  Selector_List* Eval::operator()(Selector_Schema* s)
  {
    To_String to_string;
    // the parser will look for a brace to end the selector
    std::string result_str(s->contents()->perform(this)->perform(&to_string) + "{");
    Parser p = Parser::from_c_str(result_str.c_str(), ctx, s->pstate());
    return operator()(p.parse_selector_list(exp.block_stack.back()->is_root()));
  }

  Expression* Eval::operator()(Parent_Selector* p)
  {
    Selector_List* pr = selector();
    if (pr) {
      exp.selector_stack.pop_back();
      pr = operator()(pr);
      exp.selector_stack.push_back(pr);
      return pr;
    } else {
      return SASS_MEMORY_NEW(ctx.mem, Null, p->pstate());
    }
  }

}
