#include "sass.hpp"
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
#include "sass_functions.hpp"

namespace Sass {

  inline double add(double x, double y) { return x + y; }
  inline double sub(double x, double y) { return x - y; }
  inline double mul(double x, double y) { return x * y; }
  inline double div(double x, double y) { return x / y; } // x/0 checked by caller
  inline double mod(double x, double y) { // x/0 checked by caller
    if ((x > 0 && y < 0) || (x < 0 && y > 0)) {
      double ret = std::fmod(x, y);
      return ret ? ret + y : ret;
    } else {
      return std::fmod(x, y);
    }
  }
  typedef double (*bop)(double, double);
  bop ops[Sass_OP::NUM_OPS] = {
    0, 0, // and, or
    0, 0, 0, 0, 0, 0, // eq, neq, gt, gte, lt, lte
    add, sub, mul, div, mod
  };

  Eval::Eval(Expand& exp)
  : exp(exp),
    ctx(exp.ctx),
    force(false),
    is_in_comment(false)
  { }
  Eval::~Eval() { }

  Env* Eval::environment()
  {
    return exp.environment();
  }

  Selector_List_Obj Eval::selector()
  {
    return exp.selector();
  }

  Backtrace* Eval::backtrace()
  {
    return exp.backtrace();
  }

  Expression_Ptr Eval::operator()(Block_Ptr b)
  {
    Expression_Ptr val = 0;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      val = b->at(i)->perform(this);
      if (val) return val;
    }
    return val;
  }

  Expression_Ptr Eval::operator()(Assignment_Ptr a)
  {
    Env* env = exp.environment();
    std::string var(a->variable());
    if (a->is_global()) {
      if (a->is_default()) {
        if (env->has_global(var)) {
          Expression_Ptr e = Cast<Expression>(env->get_global(var));
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
            if (AST_Node_Obj node = cur->get_local(var)) {
              Expression_Ptr e = Cast<Expression>(node);
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
        if (AST_Node_Obj node = env->get_global(var)) {
          Expression_Ptr e = Cast<Expression>(node);
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

  Expression_Ptr Eval::operator()(If_Ptr i)
  {
    Expression_Obj rv = 0;
    Env env(exp.environment());
    exp.env_stack.push_back(&env);
    Expression_Obj cond = i->predicate()->perform(this);
    if (!cond->is_false()) {
      rv = i->block()->perform(this);
    }
    else {
      Block_Obj alt = i->alternative();
      if (alt) rv = alt->perform(this);
    }
    exp.env_stack.pop_back();
    return rv.detach();
  }

  // For does not create a new env scope
  // But iteration vars are reset afterwards
  Expression_Ptr Eval::operator()(For_Ptr f)
  {
    std::string variable(f->variable());
    Expression_Obj low = f->lower_bound()->perform(this);
    if (low->concrete_type() != Expression::NUMBER) {
      throw Exception::TypeMismatch(*low, "integer");
    }
    Expression_Obj high = f->upper_bound()->perform(this);
    if (high->concrete_type() != Expression::NUMBER) {
      throw Exception::TypeMismatch(*high, "integer");
    }
    Number_Obj sass_start = Cast<Number>(low);
    Number_Obj sass_end = Cast<Number>(high);
    // check if units are valid for sequence
    if (sass_start->unit() != sass_end->unit()) {
      std::stringstream msg; msg << "Incompatible units: '"
        << sass_end->unit() << "' and '"
        << sass_start->unit() << "'.";
      error(msg.str(), low->pstate(), backtrace());
    }
    double start = sass_start->value();
    double end = sass_end->value();
    // only create iterator once in this environment
    Env env(environment(), true);
    exp.env_stack.push_back(&env);
    Number_Ptr it = SASS_MEMORY_NEW(Number, low->pstate(), start, sass_end->unit());
    env.set_local(variable, it);
    Block_Obj body = f->block();
    Expression_Ptr val = 0;
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start;
           i < end;
           ++i) {
        it->value(i);
        env.set_local(variable, it);
        val = body->perform(this);
        if (val) break;
      }
    } else {
      if (f->is_inclusive()) --end;
      for (double i = start;
           i > end;
           --i) {
        it->value(i);
        env.set_local(variable, it);
        val = body->perform(this);
        if (val) break;
      }
    }
    exp.env_stack.pop_back();
    return val;
  }

  // Eval does not create a new env scope
  // But iteration vars are reset afterwards
  Expression_Ptr Eval::operator()(Each_Ptr e)
  {
    std::vector<std::string> variables(e->variables());
    Expression_Obj expr = e->list()->perform(this);
    Env env(environment(), true);
    exp.env_stack.push_back(&env);
    List_Obj list = 0;
    Map_Ptr map = 0;
    if (expr->concrete_type() == Expression::MAP) {
      map = Cast<Map>(expr);
    }
    else if (Selector_List_Ptr ls = Cast<Selector_List>(expr)) {
      Listize listize;
      Expression_Obj rv = ls->perform(&listize);
      list = Cast<List>(rv);
    }
    else if (expr->concrete_type() != Expression::LIST) {
      list = SASS_MEMORY_NEW(List, expr->pstate(), 1, SASS_COMMA);
      list->append(expr);
    }
    else {
      list = Cast<List>(expr);
    }

    Block_Obj body = e->block();
    Expression_Obj val = 0;

    if (map) {
      for (Expression_Obj key : map->keys()) {
        Expression_Obj value = map->at(key);

        if (variables.size() == 1) {
          List_Ptr variable = SASS_MEMORY_NEW(List, map->pstate(), 2, SASS_SPACE);
          variable->append(key);
          variable->append(value);
          env.set_local(variables[0], variable);
        } else {
          env.set_local(variables[0], key);
          env.set_local(variables[1], value);
        }

        val = body->perform(this);
        if (val) break;
      }
    }
    else {
      if (list->length() == 1 && Cast<Selector_List>(list)) {
        list = Cast<List>(list);
      }
      for (size_t i = 0, L = list->length(); i < L; ++i) {
        Expression_Ptr e = list->at(i);
        // unwrap value if the expression is an argument
        if (Argument_Ptr arg = Cast<Argument>(e)) e = arg->value();
        // check if we got passed a list of args (investigate)
        if (List_Ptr scalars = Cast<List>(e)) {
          if (variables.size() == 1) {
            Expression_Ptr var = scalars;
            env.set_local(variables[0], var);
          } else {
            // XXX: this is never hit via spec tests
            for (size_t j = 0, K = variables.size(); j < K; ++j) {
              Expression_Ptr res = j >= scalars->length()
                ? SASS_MEMORY_NEW(Null, expr->pstate())
                : scalars->at(j);
              env.set_local(variables[j], res);
            }
          }
        } else {
          if (variables.size() > 0) {
            env.set_local(variables.at(0), e);
            for (size_t j = 1, K = variables.size(); j < K; ++j) {
              // XXX: this is never hit via spec tests
              Expression_Ptr res = SASS_MEMORY_NEW(Null, expr->pstate());
              env.set_local(variables[j], res);
            }
          }
        }
        val = body->perform(this);
        if (val) break;
      }
    }
    exp.env_stack.pop_back();
    return val.detach();
  }

  Expression_Ptr Eval::operator()(While_Ptr w)
  {
    Expression_Obj pred = w->predicate();
    Block_Obj body = w->block();
    Env env(environment(), true);
    exp.env_stack.push_back(&env);
    Expression_Obj cond = pred->perform(this);
    while (!cond->is_false()) {
      Expression_Obj val = body->perform(this);
      if (val) {
        exp.env_stack.pop_back();
        return val.detach();
      }
      cond = pred->perform(this);
    }
    exp.env_stack.pop_back();
    return 0;
  }

  Expression_Ptr Eval::operator()(Return_Ptr r)
  {
    return r->value()->perform(this);
  }

  Expression_Ptr Eval::operator()(Warning_Ptr w)
  {
    Sass_Output_Style outstyle = ctx.c_options.output_style;
    ctx.c_options.output_style = NESTED;
    Expression_Obj message = w->message()->perform(this);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@warn[f]")) {

      // add call stack entry
      ctx.callee_stack.push_back({
        "@warn",
        w->pstate().path,
        w->pstate().line + 1,
        w->pstate().column + 1,
        SASS_CALLEE_FUNCTION,
        { env }
      });

      Definition_Ptr def = Cast<Definition>((*env)["@warn[f]"]);
      // Block_Obj          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA, false);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_compiler);
      ctx.c_options.output_style = outstyle;
      ctx.callee_stack.pop_back();
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    std::string result(unquote(message->to_sass()));
    Backtrace top(backtrace(), w->pstate(), "");
    std::cerr << "WARNING: " << result;
    std::cerr << top.to_string();
    std::cerr << std::endl << std::endl;
    ctx.c_options.output_style = outstyle;
    return 0;
  }

  Expression_Ptr Eval::operator()(Error_Ptr e)
  {
    Sass_Output_Style outstyle = ctx.c_options.output_style;
    ctx.c_options.output_style = NESTED;
    Expression_Obj message = e->message()->perform(this);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@error[f]")) {

      // add call stack entry
      ctx.callee_stack.push_back({
        "@error",
        e->pstate().path,
        e->pstate().line + 1,
        e->pstate().column + 1,
        SASS_CALLEE_FUNCTION,
        { env }
      });

      Definition_Ptr def = Cast<Definition>((*env)["@error[f]"]);
      // Block_Obj          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA, false);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_compiler);
      ctx.c_options.output_style = outstyle;
      ctx.callee_stack.pop_back();
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    std::string result(unquote(message->to_sass()));
    ctx.c_options.output_style = outstyle;
    error(result, e->pstate());
    return 0;
  }

  Expression_Ptr Eval::operator()(Debug_Ptr d)
  {
    Sass_Output_Style outstyle = ctx.c_options.output_style;
    ctx.c_options.output_style = NESTED;
    Expression_Obj message = d->value()->perform(this);
    Env* env = exp.environment();

    // try to use generic function
    if (env->has("@debug[f]")) {

      // add call stack entry
      ctx.callee_stack.push_back({
        "@debug",
        d->pstate().path,
        d->pstate().line + 1,
        d->pstate().column + 1,
        SASS_CALLEE_FUNCTION,
        { env }
      });

      Definition_Ptr def = Cast<Definition>((*env)["@debug[f]"]);
      // Block_Obj          body   = def->block();
      // Native_Function func   = def->native_function();
      Sass_Function_Entry c_function = def->c_function();
      Sass_Function_Fn c_func = sass_function_get_function(c_function);

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(1, SASS_COMMA, false);
      sass_list_set_value(c_args, 0, message->perform(&to_c));
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_compiler);
      ctx.c_options.output_style = outstyle;
      ctx.callee_stack.pop_back();
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      return 0;

    }

    std::string cwd(ctx.cwd());
    std::string result(unquote(message->to_sass()));
    std::string abs_path(Sass::File::rel2abs(d->pstate().path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(d->pstate().path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, abs_path, d->pstate().path));
    ctx.c_options.output_style = outstyle;

    std::cerr << output_path << ":" << d->pstate().line+1 << " DEBUG: " << result;
    std::cerr << std::endl;
    return 0;
  }

  Expression_Ptr Eval::operator()(List_Ptr l)
  {
    // special case for unevaluated map
    if (l->separator() == SASS_HASH) {
      Map_Obj lm = SASS_MEMORY_NEW(Map,
                                l->pstate(),
                                l->length() / 2);
      for (size_t i = 0, L = l->length(); i < L; i += 2)
      {
        Expression_Obj key = (*l)[i+0]->perform(this);
        Expression_Obj val = (*l)[i+1]->perform(this);
        // make sure the color key never displays its real name
        key->is_delayed(true); // verified
        *lm << std::make_pair(key, val);
      }
      if (lm->has_duplicate_key()) {
        throw Exception::DuplicateKeyError(*lm, *l);
      }

      lm->is_interpolant(l->is_interpolant());
      return lm->perform(this);
    }
    // check if we should expand it
    if (l->is_expanded()) return l;
    // regular case for unevaluated lists
    List_Obj ll = SASS_MEMORY_NEW(List,
                               l->pstate(),
                               l->length(),
                               l->separator(),
                               l->is_arglist(),
                               l->is_bracketed());
    for (size_t i = 0, L = l->length(); i < L; ++i) {
      ll->append((*l)[i]->perform(this));
    }
    ll->is_interpolant(l->is_interpolant());
    ll->from_selector(l->from_selector());
    ll->is_expanded(true);
    return ll.detach();
  }

  Expression_Ptr Eval::operator()(Map_Ptr m)
  {
    if (m->is_expanded()) return m;

    // make sure we're not starting with duplicate keys.
    // the duplicate key state will have been set in the parser phase.
    if (m->has_duplicate_key()) {
      throw Exception::DuplicateKeyError(*m, *m);
    }

    Map_Obj mm = SASS_MEMORY_NEW(Map,
                                m->pstate(),
                                m->length());
    for (auto key : m->keys()) {
      Expression_Ptr ex_key = key->perform(this);
      Expression_Ptr ex_val = m->at(key)->perform(this);
      *mm << std::make_pair(ex_key, ex_val);
    }

    // check the evaluated keys aren't duplicates.
    if (mm->has_duplicate_key()) {
      throw Exception::DuplicateKeyError(*mm, *m);
    }

    mm->is_expanded(true);
    return mm.detach();
  }

  Expression_Ptr Eval::operator()(Binary_Expression_Ptr b_in)
  {

    String_Schema_Obj ret_schema;
    Binary_Expression_Obj b = b_in;
    enum Sass_OP op_type = b->optype();

    // only the last item will be used to eval the binary expression
    if (String_Schema_Ptr s_l = Cast<String_Schema>(b->left())) {
      if (!s_l->has_interpolant() && (!s_l->is_right_interpolant())) {
        ret_schema = SASS_MEMORY_NEW(String_Schema, b->pstate());
        Binary_Expression_Obj bin_ex = SASS_MEMORY_NEW(Binary_Expression, b->pstate(),
                                                    b->op(), s_l->last(), b->right());
        bin_ex->is_delayed(b->left()->is_delayed() || b->right()->is_delayed()); // unverified
        for (size_t i = 0; i < s_l->length() - 1; ++i) {
          ret_schema->append(s_l->at(i)->perform(this));
        }
        ret_schema->append(bin_ex->perform(this));
        return ret_schema->perform(this);
      }
    }
    if (String_Schema_Ptr s_r = Cast<String_Schema>(b->right())) {

      if (!s_r->has_interpolant() && (!s_r->is_left_interpolant() || op_type == Sass_OP::DIV)) {
        ret_schema = SASS_MEMORY_NEW(String_Schema, b->pstate());
        Binary_Expression_Obj bin_ex = SASS_MEMORY_NEW(Binary_Expression, b->pstate(),
                                                    b->op(), b->left(), s_r->first());
        bin_ex->is_delayed(b->left()->is_delayed() || b->right()->is_delayed()); // verified
        ret_schema->append(bin_ex->perform(this));
        for (size_t i = 1; i < s_r->length(); ++i) {
          ret_schema->append(s_r->at(i)->perform(this));
        }
        return ret_schema->perform(this);
      }
    }

    // don't eval delayed expressions (the '/' when used as a separator)
    if (!force && op_type == Sass_OP::DIV && b->is_delayed()) {
      b->right(b->right()->perform(this));
      b->left(b->left()->perform(this));
      return b.detach();
    }

    Expression_Obj lhs = b->left();
    Expression_Obj rhs = b->right();

    // fully evaluate their values
    if (op_type == Sass_OP::EQ ||
        op_type == Sass_OP::NEQ ||
        op_type == Sass_OP::GT ||
        op_type == Sass_OP::GTE ||
        op_type == Sass_OP::LT ||
        op_type == Sass_OP::LTE)
    {
      LOCAL_FLAG(force, true);
      lhs->is_expanded(false);
      lhs->set_delayed(false);
      lhs = lhs->perform(this);
      rhs->is_expanded(false);
      rhs->set_delayed(false);
      rhs = rhs->perform(this);
    }
    else {
      lhs = lhs->perform(this);
    }

    Binary_Expression_Obj u3 = b;
    switch (op_type) {
      case Sass_OP::AND: {
        return *lhs ? b->right()->perform(this) : lhs.detach();
      } break;

      case Sass_OP::OR: {
        return *lhs ? lhs.detach() : b->right()->perform(this);
      } break;

      default:
        break;
    }
    // not a logical connective, so go ahead and eval the rhs
    rhs = rhs->perform(this);
    AST_Node_Obj lu = lhs;
    AST_Node_Obj ru = rhs;

    Expression::Concrete_Type l_type = lhs->concrete_type();
    Expression::Concrete_Type r_type = rhs->concrete_type();

    // Is one of the operands an interpolant?
    String_Schema_Obj s1 = Cast<String_Schema>(b->left());
    String_Schema_Obj s2 = Cast<String_Schema>(b->right());
    Binary_Expression_Obj b1 = Cast<Binary_Expression>(b->left());
    Binary_Expression_Obj b2 = Cast<Binary_Expression>(b->right());

    bool schema_op = false;

    bool force_delay = (s2 && s2->is_left_interpolant()) ||
                       (s1 && s1->is_right_interpolant()) ||
                       (b1 && b1->is_right_interpolant()) ||
                       (b2 && b2->is_left_interpolant());

    if ((s1 && s1->has_interpolants()) || (s2 && s2->has_interpolants()) || force_delay)
    {
      if (op_type == Sass_OP::DIV || op_type == Sass_OP::MUL || op_type == Sass_OP::MOD || op_type == Sass_OP::ADD || op_type == Sass_OP::SUB ||
          op_type == Sass_OP::EQ) {
        // If possible upgrade LHS to a number (for number to string compare)
        if (String_Constant_Ptr str = Cast<String_Constant>(lhs)) {
          std::string value(str->value());
          const char* start = value.c_str();
          if (Prelexer::sequence < Prelexer::dimension, Prelexer::end_of_file >(start) != 0) {
            Textual_Obj l = SASS_MEMORY_NEW(Textual, b->pstate(), Textual::DIMENSION, str->value());
            lhs = l->perform(this);
          }
        }
        // If possible upgrade RHS to a number (for string to number compare)
        if (String_Constant_Ptr str = Cast<String_Constant>(rhs)) {
          std::string value(str->value());
          const char* start = value.c_str();
          if (Prelexer::sequence < Prelexer::dimension, Prelexer::number >(start) != 0) {
            Textual_Obj r = SASS_MEMORY_NEW(Textual, b->pstate(), Textual::DIMENSION, str->value());
            rhs = r->perform(this);
          }
        }
      }

      To_Value to_value(ctx);
      Value_Obj v_l = Cast<Value>(lhs->perform(&to_value));
      Value_Obj v_r = Cast<Value>(rhs->perform(&to_value));
      l_type = lhs->concrete_type();
      r_type = rhs->concrete_type();

      if (s2 && s2->has_interpolants() && s2->length()) {
        Textual_Obj front = Cast<Textual>(s2->elements().front());
        if (front && !front->is_interpolant())
        {
          // XXX: this is never hit via spec tests
          schema_op = true;
          rhs = front->perform(this);
        }
      }

      if (force_delay) {
        std::string str("");
        str += v_l->to_string(ctx.c_options);
        if (b->op().ws_before) str += " ";
        str += b->separator();
        if (b->op().ws_after) str += " ";
        str += v_r->to_string(ctx.c_options);
        String_Constant_Ptr val = SASS_MEMORY_NEW(String_Constant, b->pstate(), str);
        val->is_interpolant(b->left()->has_interpolant());
        return val;
      }
    }

    // see if it's a relational expression
    try {
      switch(op_type) {
        case Sass_OP::EQ:  return SASS_MEMORY_NEW(Boolean, b->pstate(), eq(lhs, rhs));
        case Sass_OP::NEQ: return SASS_MEMORY_NEW(Boolean, b->pstate(), !eq(lhs, rhs));
        case Sass_OP::GT:  return SASS_MEMORY_NEW(Boolean, b->pstate(), !lt(lhs, rhs, "gt") && !eq(lhs, rhs));
        case Sass_OP::GTE: return SASS_MEMORY_NEW(Boolean, b->pstate(), !lt(lhs, rhs, "gte"));
        case Sass_OP::LT:  return SASS_MEMORY_NEW(Boolean, b->pstate(), lt(lhs, rhs, "lt"));
        case Sass_OP::LTE: return SASS_MEMORY_NEW(Boolean, b->pstate(), lt(lhs, rhs, "lte") || eq(lhs, rhs));
        default:                     break;
      }
    }
    catch (Exception::OperationError& err)
    {
      // throw Exception::Base(b->pstate(), err.what());
      throw Exception::SassValueError(b->pstate(), err);
    }

    l_type = lhs->concrete_type();
    r_type = rhs->concrete_type();

    // ToDo: throw error in op functions
    // ToDo: then catch and re-throw them
    Expression_Obj rv = 0;
    try {
      ParserState pstate(b->pstate());
      if (l_type == Expression::NUMBER && r_type == Expression::NUMBER) {
        Number_Ptr l_n = Cast<Number>(lhs);
        Number_Ptr r_n = Cast<Number>(rhs);
        rv = op_numbers(op_type, *l_n, *r_n, ctx.c_options, &pstate);
      }
      else if (l_type == Expression::NUMBER && r_type == Expression::COLOR) {
        Number_Ptr l_n = Cast<Number>(lhs);
        Color_Ptr r_c = Cast<Color>(rhs);
        rv = op_number_color(op_type, *l_n, *r_c, ctx.c_options, &pstate);
      }
      else if (l_type == Expression::COLOR && r_type == Expression::NUMBER) {
        Color_Ptr l_c = Cast<Color>(lhs);
        Number_Ptr r_n = Cast<Number>(rhs);
        rv = op_color_number(op_type, *l_c, *r_n, ctx.c_options, &pstate);
      }
      else if (l_type == Expression::COLOR && r_type == Expression::COLOR) {
        Color_Ptr l_c = Cast<Color>(lhs);
        Color_Ptr r_c = Cast<Color>(rhs);
        rv = op_colors(op_type, *l_c, *r_c, ctx.c_options, &pstate);
      }
      else {
        To_Value to_value(ctx);
        // this will leak if perform does not return a value!
        Value_Obj v_l = Cast<Value>(lhs->perform(&to_value));
        Value_Obj v_r = Cast<Value>(rhs->perform(&to_value));
        bool interpolant = b->is_right_interpolant() ||
                           b->is_left_interpolant() ||
                           b->is_interpolant();
        if (op_type == Sass_OP::SUB) interpolant = false;
        // if (op_type == Sass_OP::DIV) interpolant = true;
        // check for type violations
        if (l_type == Expression::MAP) {
          throw Exception::InvalidValue(*v_l);
        }
        if (r_type == Expression::MAP) {
          throw Exception::InvalidValue(*v_r);
        }
        Value_Ptr ex = op_strings(b->op(), *v_l, *v_r, ctx.c_options, &pstate, !interpolant); // pass true to compress
        if (String_Constant_Ptr str = Cast<String_Constant>(ex))
        {
          if (str->concrete_type() == Expression::STRING)
          {
            String_Constant_Ptr lstr = Cast<String_Constant>(lhs);
            String_Constant_Ptr rstr = Cast<String_Constant>(rhs);
            if (op_type != Sass_OP::SUB) {
              if (String_Constant_Ptr org = lstr ? lstr : rstr)
              { str->quote_mark(org->quote_mark()); }
            }
          }
        }
        ex->is_interpolant(b->is_interpolant());
        rv = ex;
      }
    }
    catch (Exception::OperationError& err)
    {
      // throw Exception::Base(b->pstate(), err.what());
      throw Exception::SassValueError(b->pstate(), err);
    }

    if (rv) {
      if (schema_op) {
        // XXX: this is never hit via spec tests
        (*s2)[0] = rv;
        rv = s2->perform(this);
      }
    }

    return rv.detach();

  }

  Expression_Ptr Eval::operator()(Unary_Expression_Ptr u)
  {
    Expression_Obj operand = u->operand()->perform(this);
    if (u->optype() == Unary_Expression::NOT) {
      Boolean_Ptr result = SASS_MEMORY_NEW(Boolean, u->pstate(), (bool)*operand);
      result->value(!result->value());
      return result;
    }
    else if (Number_Obj nr = Cast<Number>(operand)) {
      // negate value for minus unary expression
      if (u->optype() == Unary_Expression::MINUS) {
        Number_Obj cpy = SASS_MEMORY_COPY(nr);
        cpy->value( - cpy->value() ); // negate value
        return cpy.detach(); // return the copy
      }
      // nothing for positive
      return nr.detach();
    }
    else {
      // Special cases: +/- variables which evaluate to null ouput just +/-,
      // but +/- null itself outputs the string
      if (operand->concrete_type() == Expression::NULL_VAL && Cast<Variable>(u->operand())) {
        u->operand(SASS_MEMORY_NEW(String_Quoted, u->pstate(), ""));
      }
      // Never apply unary opertions on colors @see #2140
      else if (Color_Ptr color = Cast<Color>(operand)) {
        // Use the color name if this was eval with one
        if (color->disp().length() > 0) {
          operand = SASS_MEMORY_NEW(String_Constant, operand->pstate(), color->disp());
          u->operand(operand);
        }
      }
      else {
        u->operand(operand);
      }

      return SASS_MEMORY_NEW(String_Quoted,
                             u->pstate(),
                             u->inspect());
    }
    // unreachable
    return u;
  }

  Expression_Ptr Eval::operator()(Function_Call_Ptr c)
  {
    if (backtrace()->parent != NULL && backtrace()->depth() > Constants::MaxCallStack) {
        // XXX: this is never hit via spec tests
        std::ostringstream stm;
        stm << "Stack depth exceeded max of " << Constants::MaxCallStack;
        error(stm.str(), c->pstate(), backtrace());
    }
    std::string name(Util::normalize_underscores(c->name()));
    std::string full_name(name + "[f]");
    // we make a clone here, need to implement that further
    Arguments_Obj args = c->arguments();

    Env* env = environment();
    if (!env->has(full_name) || (!c->via_call() && Prelexer::re_special_fun(name.c_str()))) {
      if (!env->has("*[f]")) {
        for (Argument_Obj arg : args->elements()) {
          if (List_Obj ls = Cast<List>(arg->value())) {
            if (ls->size() == 0) error("() isn't a valid CSS value.", c->pstate());
          }
        }
        args = Cast<Arguments>(args->perform(this));
        Function_Call_Obj lit = SASS_MEMORY_NEW(Function_Call,
                                             c->pstate(),
                                             c->name(),
                                             args);
        if (args->has_named_arguments()) {
          error("Function " + c->name() + " doesn't support keyword arguments", c->pstate());
        }
        String_Quoted_Ptr str = SASS_MEMORY_NEW(String_Quoted,
                                             c->pstate(),
                                             lit->to_string(ctx.c_options));
        str->is_interpolant(c->is_interpolant());
        return str;
      } else {
        // call generic function
        full_name = "*[f]";
      }
    }

    // further delay for calls
    if (full_name != "call[f]") {
      args->set_delayed(false); // verified
    }
    if (full_name != "if[f]") {
      args = Cast<Arguments>(args->perform(this));
    }
    Definition_Ptr def = Cast<Definition>((*env)[full_name]);

    if (def->is_overload_stub()) {
      std::stringstream ss;
      size_t L = args->length();
      // account for rest arguments
      if (args->has_rest_argument() && args->length() > 0) {
        // get the rest arguments list
        List_Ptr rest = Cast<List>(args->last()->value());
        // arguments before rest argument plus rest
        if (rest) L += rest->length() - 1;
      }
      ss << full_name << L;
      full_name = ss.str();
      std::string resolved_name(full_name);
      if (!env->has(resolved_name)) error("overloaded function `" + std::string(c->name()) + "` given wrong number of arguments", c->pstate());
      def = Cast<Definition>((*env)[resolved_name]);
    }

    Expression_Obj     result = c;
    Block_Obj          body   = def->block();
    Native_Function func   = def->native_function();
    Sass_Function_Entry c_function = def->c_function();

    Parameters_Obj params = def->parameters();
    Env fn_env(def->environment());
    exp.env_stack.push_back(&fn_env);

    if (func || body) {
      bind(std::string("Function"), c->name(), params, args, &ctx, &fn_env, this);
      Backtrace here(backtrace(), c->pstate(), ", in function `" + c->name() + "`");
      exp.backtrace_stack.push_back(&here);
      ctx.callee_stack.push_back({
        c->name().c_str(),
        c->pstate().path,
        c->pstate().line + 1,
        c->pstate().column + 1,
        SASS_CALLEE_FUNCTION,
        { env }
      });

      // eval the body if user-defined or special, invoke underlying CPP function if native
      if (body /* && !Prelexer::re_special_fun(name.c_str()) */) {
        result = body->perform(this);
      }
      else if (func) {
        result = func(fn_env, *env, ctx, def->signature(), c->pstate(), backtrace(), exp.selector_stack);
      }
      if (!result) {
        error(std::string("Function ") + c->name() + " did not return a value", c->pstate());
      }
      exp.backtrace_stack.pop_back();
      ctx.callee_stack.pop_back();
    }

    // else if it's a user-defined c function
    // convert call into C-API compatible form
    else if (c_function) {
      Sass_Function_Fn c_func = sass_function_get_function(c_function);
      if (full_name == "*[f]") {
        String_Quoted_Obj str = SASS_MEMORY_NEW(String_Quoted, c->pstate(), c->name());
        Arguments_Obj new_args = SASS_MEMORY_NEW(Arguments, c->pstate());
        new_args->append(SASS_MEMORY_NEW(Argument, c->pstate(), str));
        new_args->concat(args);
        args = new_args;
      }

      // populates env with default values for params
      std::string ff(c->name());
      bind(std::string("Function"), c->name(), params, args, &ctx, &fn_env, this);

      Backtrace here(backtrace(), c->pstate(), ", in function `" + c->name() + "`");
      exp.backtrace_stack.push_back(&here);
      ctx.callee_stack.push_back({
        c->name().c_str(),
        c->pstate().path,
        c->pstate().line + 1,
        c->pstate().column + 1,
        SASS_CALLEE_C_FUNCTION,
        { env }
      });

      To_C to_c;
      union Sass_Value* c_args = sass_make_list(params->length(), SASS_COMMA, false);
      for(size_t i = 0; i < params->length(); i++) {
        Parameter_Obj param = params->at(i);
        std::string key = param->name();
        AST_Node_Obj node = fn_env.get_local(key);
        Expression_Obj arg = Cast<Expression>(node);
        sass_list_set_value(c_args, i, arg->perform(&to_c));
      }
      union Sass_Value* c_val = c_func(c_args, c_function, ctx.c_compiler);
      if (sass_value_get_tag(c_val) == SASS_ERROR) {
        error("error in C function " + c->name() + ": " + sass_error_get_message(c_val), c->pstate(), backtrace());
      } else if (sass_value_get_tag(c_val) == SASS_WARNING) {
        error("warning in C function " + c->name() + ": " + sass_warning_get_message(c_val), c->pstate(), backtrace());
      }
      result = cval_to_astnode(c_val, backtrace(), c->pstate());

      exp.backtrace_stack.pop_back();
      ctx.callee_stack.pop_back();
      sass_delete_value(c_args);
      if (c_val != c_args)
        sass_delete_value(c_val);
    }

    // link back to function definition
    // only do this for custom functions
    if (result->pstate().file == std::string::npos)
      result->pstate(c->pstate());

    result = result->perform(this);
    result->is_interpolant(c->is_interpolant());
    exp.env_stack.pop_back();
    return result.detach();
  }

  Expression_Ptr Eval::operator()(Function_Call_Schema_Ptr s)
  {
    Expression_Ptr evaluated_name = s->name()->perform(this);
    Expression_Ptr evaluated_args = s->arguments()->perform(this);
    String_Schema_Obj ss = SASS_MEMORY_NEW(String_Schema, s->pstate(), 2);
    ss->append(evaluated_name);
    ss->append(evaluated_args);
    return ss->perform(this);
  }

  Expression_Ptr Eval::operator()(Variable_Ptr v)
  {
    std::string name(v->name());
    Expression_Obj value = 0;
    Env* env = environment();
    if (env->has(name)) {
      value = Cast<Expression>((*env)[name]);
    }
    else error("Undefined variable: \"" + v->name() + "\".", v->pstate());
    if (Argument* arg = Cast<Argument>(value)) {
      value = arg->value();
    }

    // behave according to as ruby sass (add leading zero)
    if (Number_Ptr nr = Cast<Number>(value)) {
      nr->zero(true);
    }

    value->is_interpolant(v->is_interpolant());
    if (force) value->is_expanded(false);
    value->set_delayed(false); // verified
    value = value->perform(this);
    if(!force) (*env)[name] = value;
    return value.detach();
  }

  Expression_Ptr Eval::operator()(Textual_Ptr t)
  {
    using Prelexer::number;
    Expression_Obj result = 0;
    size_t L = t->value().length();
    bool zero = !( (L > 0 && t->value().substr(0, 1) == ".") ||
                   (L > 1 && t->value().substr(0, 2) == "0.") ||
                   (L > 1 && t->value().substr(0, 2) == "-.")  ||
                   (L > 2 && t->value().substr(0, 3) == "-0.")
                 );

    const std::string& text = t->value();
    size_t num_pos = text.find_first_not_of(" \n\r\t");
    if (num_pos == std::string::npos) num_pos = text.length();
    size_t unit_pos = text.find_first_not_of("-+0123456789.", num_pos);
    if (unit_pos == std::string::npos) unit_pos = text.length();
    const std::string& num = text.substr(num_pos, unit_pos - num_pos);

    switch (t->valtype())
    {
      case Textual::NUMBER:
        result = SASS_MEMORY_NEW(Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 "",
                                 zero);
        break;
      case Textual::PERCENTAGE:
        result = SASS_MEMORY_NEW(Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 "%",
                                 true);
        break;
      case Textual::DIMENSION:
        result = SASS_MEMORY_NEW(Number,
                                 t->pstate(),
                                 sass_atof(num.c_str()),
                                 Token(number(text.c_str())),
                                 zero);
        break;
      case Textual::HEX: {
        if (t->value().substr(0, 1) != "#") {
          result = SASS_MEMORY_NEW(String_Quoted, t->pstate(), t->value());
          break;
        }
        std::string hext(t->value().substr(1)); // chop off the '#'
        if (hext.length() == 6) {
          std::string r(hext.substr(0,2));
          std::string g(hext.substr(2,2));
          std::string b(hext.substr(4,2));
          result = SASS_MEMORY_NEW(Color,
                                   t->pstate(),
                                   static_cast<double>(strtol(r.c_str(), NULL, 16)),
                                   static_cast<double>(strtol(g.c_str(), NULL, 16)),
                                   static_cast<double>(strtol(b.c_str(), NULL, 16)),
                                   1, // alpha channel
                                   t->value());
        }
        else {
          result = SASS_MEMORY_NEW(Color,
                                   t->pstate(),
                                   static_cast<double>(strtol(std::string(2,hext[0]).c_str(), NULL, 16)),
                                   static_cast<double>(strtol(std::string(2,hext[1]).c_str(), NULL, 16)),
                                   static_cast<double>(strtol(std::string(2,hext[2]).c_str(), NULL, 16)),
                                   1, // alpha channel
                                   t->value());
        }
      } break;
    }
    result->is_interpolant(t->is_interpolant());
    return result.detach();
  }

  Expression_Ptr Eval::operator()(Color_Ptr c)
  {
    return c;
  }

  Expression_Ptr Eval::operator()(Number_Ptr n)
  {
    return n;
  }

  Expression_Ptr Eval::operator()(Boolean_Ptr b)
  {
    return b;
  }

  void Eval::interpolation(Context& ctx, std::string& res, Expression_Obj ex, bool into_quotes, bool was_itpl) {

    bool needs_closing_brace = false;

    if (Arguments_Ptr args = Cast<Arguments>(ex)) {
      List_Ptr ll = SASS_MEMORY_NEW(List, args->pstate(), 0, SASS_COMMA);
      for(auto arg : args->elements()) {
        ll->append(arg->value());
      }
      ll->is_interpolant(args->is_interpolant());
      needs_closing_brace = true;
      res += "(";
      ex = ll;
    }
    if (Number_Ptr nr = Cast<Number>(ex)) {
      if (!nr->is_valid_css_unit()) {
        throw Exception::InvalidValue(*nr);
      }
    }
    if (Argument_Ptr arg = Cast<Argument>(ex)) {
      ex = arg->value();
    }
    if (String_Quoted_Ptr sq = Cast<String_Quoted>(ex)) {
      if (was_itpl) {
        bool was_interpolant = ex->is_interpolant();
        ex = SASS_MEMORY_NEW(String_Constant, sq->pstate(), sq->value());
        ex->is_interpolant(was_interpolant);
      }
    }

    if (Cast<Null>(ex)) { return; }

    // parent selector needs another go
    if (Cast<Parent_Selector>(ex)) {
      // XXX: this is never hit via spec tests
      ex = ex->perform(this);
    }

    if (List_Ptr l = Cast<List>(ex)) {
      List_Obj ll = SASS_MEMORY_NEW(List, l->pstate(), 0, l->separator());
      // this fixes an issue with bourbon sample, not really sure why
      // if (l->size() && Cast<Null>((*l)[0])) { res += ""; }
      for(Expression_Obj item : *l) {
        item->is_interpolant(l->is_interpolant());
        std::string rl(""); interpolation(ctx, rl, item, into_quotes, l->is_interpolant());
        bool is_null = Cast<Null>(item) != 0; // rl != ""
        if (!is_null) ll->append(SASS_MEMORY_NEW(String_Quoted, item->pstate(), rl));
      }
      // Check indicates that we probably should not get a list
      // here. Normally single list items are already unwrapped.
      if (l->size() > 1) {
        // string_to_output would fail "#{'_\a' '_\a'}";
        std::string str(ll->to_string(ctx.c_options));
        newline_to_space(str); // replace directly
        res += str; // append to result string
      } else {
        res += (ll->to_string(ctx.c_options));
      }
      ll->is_interpolant(l->is_interpolant());
    }

    // Value
    // Textual
    // Function_Call
    // Selector_List
    // String_Quoted
    // String_Constant
    // Parent_Selector
    // Binary_Expression
    else {
      // ex = ex->perform(this);
      if (into_quotes && ex->is_interpolant()) {
        res += evacuate_escapes(ex ? ex->to_string(ctx.c_options) : "");
      } else {
        res += ex ? ex->to_string(ctx.c_options) : "";
      }
    }

    if (needs_closing_brace) res += ")";

  }

  Expression_Ptr Eval::operator()(String_Schema_Ptr s)
  {
    size_t L = s->length();
    bool into_quotes = false;
    if (L > 1) {
      if (!Cast<String_Quoted>((*s)[0]) && !Cast<String_Quoted>((*s)[L - 1])) {
      if (String_Constant_Ptr l = Cast<String_Constant>((*s)[0])) {
        if (String_Constant_Ptr r = Cast<String_Constant>((*s)[L - 1])) {
          if (r->value().size() > 0) {
            if (l->value()[0] == '"' && r->value()[r->value().size() - 1] == '"') into_quotes = true;
            if (l->value()[0] == '\'' && r->value()[r->value().size() - 1] == '\'') into_quotes = true;
          }
        }
      }
      }
    }
    bool was_quoted = false;
    bool was_interpolant = false;
    std::string res("");
    for (size_t i = 0; i < L; ++i) {
      bool is_quoted = Cast<String_Quoted>((*s)[i]) != NULL;
      if (was_quoted && !(*s)[i]->is_interpolant() && !was_interpolant) { res += " "; }
      else if (i > 0 && is_quoted && !(*s)[i]->is_interpolant() && !was_interpolant) { res += " "; }
      Expression_Obj ex = (*s)[i]->perform(this);
      interpolation(ctx, res, ex, into_quotes, ex->is_interpolant());
      was_quoted = Cast<String_Quoted>((*s)[i]) != NULL;
      was_interpolant = (*s)[i]->is_interpolant();

    }
    if (!s->is_interpolant()) {
      if (s->length() > 1 && res == "") return SASS_MEMORY_NEW(Null, s->pstate());
      return SASS_MEMORY_NEW(String_Constant, s->pstate(), res);
    }
    // string schema seems to have a special unquoting behavior (also handles "nested" quotes)
    String_Quoted_Obj str = SASS_MEMORY_NEW(String_Quoted, s->pstate(), res, 0, false, false, false);
    // if (s->is_interpolant()) str->quote_mark(0);
    // String_Constant_Ptr str = SASS_MEMORY_NEW(String_Constant, s->pstate(), res);
    if (str->quote_mark()) str->quote_mark('*');
    else if (!is_in_comment) str->value(string_to_output(str->value()));
    str->is_interpolant(s->is_interpolant());
    return str.detach();
  }


  Expression_Ptr Eval::operator()(String_Constant_Ptr s)
  {
    if (!s->is_delayed() && name_to_color(s->value())) {
      Color_Ptr c = SASS_MEMORY_COPY(name_to_color(s->value())); // copy
      c->pstate(s->pstate());
      c->disp(s->value());
      c->is_delayed(true);
      return c;
    }
    return s;
  }

  Expression_Ptr Eval::operator()(String_Quoted_Ptr s)
  {
    String_Quoted_Ptr str = SASS_MEMORY_NEW(String_Quoted, s->pstate(), "");
    str->value(s->value());
    str->quote_mark(s->quote_mark());
    str->is_interpolant(s->is_interpolant());
    return str;
  }

  Expression_Ptr Eval::operator()(Supports_Operator_Ptr c)
  {
    Expression_Ptr left = c->left()->perform(this);
    Expression_Ptr right = c->right()->perform(this);
    Supports_Operator_Ptr cc = SASS_MEMORY_NEW(Supports_Operator,
                                 c->pstate(),
                                 Cast<Supports_Condition>(left),
                                 Cast<Supports_Condition>(right),
                                 c->operand());
    return cc;
  }

  Expression_Ptr Eval::operator()(Supports_Negation_Ptr c)
  {
    Expression_Ptr condition = c->condition()->perform(this);
    Supports_Negation_Ptr cc = SASS_MEMORY_NEW(Supports_Negation,
                                 c->pstate(),
                                 Cast<Supports_Condition>(condition));
    return cc;
  }

  Expression_Ptr Eval::operator()(Supports_Declaration_Ptr c)
  {
    Expression_Ptr feature = c->feature()->perform(this);
    Expression_Ptr value = c->value()->perform(this);
    Supports_Declaration_Ptr cc = SASS_MEMORY_NEW(Supports_Declaration,
                              c->pstate(),
                              feature,
                              value);
    return cc;
  }

  Expression_Ptr Eval::operator()(Supports_Interpolation_Ptr c)
  {
    Expression_Ptr value = c->value()->perform(this);
    Supports_Interpolation_Ptr cc = SASS_MEMORY_NEW(Supports_Interpolation,
                            c->pstate(),
                            value);
    return cc;
  }

  Expression_Ptr Eval::operator()(At_Root_Query_Ptr e)
  {
    Expression_Obj feature = e->feature();
    feature = (feature ? feature->perform(this) : 0);
    Expression_Obj value = e->value();
    value = (value ? value->perform(this) : 0);
    Expression_Ptr ee = SASS_MEMORY_NEW(At_Root_Query,
                                     e->pstate(),
                                     Cast<String>(feature),
                                     value);
    return ee;
  }

  Expression_Ptr Eval::operator()(Media_Query_Ptr q)
  {
    String_Obj t = q->media_type();
    t = static_cast<String_Ptr>(t.isNull() ? 0 : t->perform(this));
    Media_Query_Obj qq = SASS_MEMORY_NEW(Media_Query,
                                      q->pstate(),
                                      t,
                                      q->length(),
                                      q->is_negated(),
                                      q->is_restricted());
    for (size_t i = 0, L = q->length(); i < L; ++i) {
      qq->append(static_cast<Media_Query_Expression_Ptr>((*q)[i]->perform(this)));
    }
    return qq.detach();
  }

  Expression_Ptr Eval::operator()(Media_Query_Expression_Ptr e)
  {
    Expression_Obj feature = e->feature();
    feature = (feature ? feature->perform(this) : 0);
    if (feature && Cast<String_Quoted>(feature)) {
      feature = SASS_MEMORY_NEW(String_Quoted,
                                  feature->pstate(),
                                  Cast<String_Quoted>(feature)->value());
    }
    Expression_Obj value = e->value();
    value = (value ? value->perform(this) : 0);
    if (value && Cast<String_Quoted>(value)) {
      // XXX: this is never hit via spec tests
      value = SASS_MEMORY_NEW(String_Quoted,
                                value->pstate(),
                                Cast<String_Quoted>(value)->value());
    }
    return SASS_MEMORY_NEW(Media_Query_Expression,
                           e->pstate(),
                           feature,
                           value,
                           e->is_interpolated());
  }

  Expression_Ptr Eval::operator()(Null_Ptr n)
  {
    return n;
  }

  Expression_Ptr Eval::operator()(Argument_Ptr a)
  {
    Expression_Obj val = a->value()->perform(this);
    bool is_rest_argument = a->is_rest_argument();
    bool is_keyword_argument = a->is_keyword_argument();

    if (a->is_rest_argument()) {
      if (val->concrete_type() == Expression::MAP) {
        is_rest_argument = false;
        is_keyword_argument = true;
      }
      else if(val->concrete_type() != Expression::LIST) {
        List_Obj wrapper = SASS_MEMORY_NEW(List,
                                        val->pstate(),
                                        0,
                                        SASS_COMMA,
                                        true);
        wrapper->append(val);
        val = wrapper;
      }
    }
    return SASS_MEMORY_NEW(Argument,
                           a->pstate(),
                           val,
                           a->name(),
                           is_rest_argument,
                           is_keyword_argument);
  }

  Expression_Ptr Eval::operator()(Arguments_Ptr a)
  {
    Arguments_Obj aa = SASS_MEMORY_NEW(Arguments, a->pstate());
    if (a->length() == 0) return aa.detach();
    for (size_t i = 0, L = a->length(); i < L; ++i) {
      Expression_Obj rv = (*a)[i]->perform(this);
      Argument_Ptr arg = Cast<Argument>(rv);
      if (!(arg->is_rest_argument() || arg->is_keyword_argument())) {
        aa->append(arg);
      }
    }

    if (a->has_rest_argument()) {
      Expression_Obj rest = a->get_rest_argument()->perform(this);
      Expression_Obj splat = Cast<Argument>(rest)->value()->perform(this);

      Sass_Separator separator = SASS_COMMA;
      List_Ptr ls = Cast<List>(splat);
      Map_Ptr ms = Cast<Map>(splat);

      List_Obj arglist = SASS_MEMORY_NEW(List,
                                      splat->pstate(),
                                      0,
                                      ls ? ls->separator() : separator,
                                      true);

      if (ls && ls->is_arglist()) {
        arglist->concat(ls);
      } else if (ms) {
        aa->append(SASS_MEMORY_NEW(Argument, splat->pstate(), ms, "", false, true));
      } else if (ls) {
        arglist->concat(ls);
      } else {
        arglist->append(splat);
      }
      if (arglist->length()) {
        aa->append(SASS_MEMORY_NEW(Argument, splat->pstate(), arglist, "", true));
      }
    }

    if (a->has_keyword_argument()) {
      Expression_Obj rv = a->get_keyword_argument()->perform(this);
      Argument_Ptr rvarg = Cast<Argument>(rv);
      Expression_Obj kwarg = rvarg->value()->perform(this);

      aa->append(SASS_MEMORY_NEW(Argument, kwarg->pstate(), kwarg, "", false, true));
    }
    return aa.detach();
  }

  Expression_Ptr Eval::operator()(Comment_Ptr c)
  {
    return 0;
  }

  inline Expression_Ptr Eval::fallback_impl(AST_Node_Ptr n)
  {
    return static_cast<Expression_Ptr>(n);
  }

  // All the binary helpers.

  bool Eval::eq(Expression_Obj lhs, Expression_Obj rhs)
  {
    // use compare operator from ast node
    return lhs && rhs && *lhs == *rhs;
  }

  bool Eval::lt(Expression_Obj lhs, Expression_Obj rhs, std::string op)
  {
    Number_Obj l = Cast<Number>(lhs);
    Number_Obj r = Cast<Number>(rhs);
    // use compare operator from ast node
    if (!l || !r) throw Exception::UndefinedOperation(lhs, rhs, op);
    // use compare operator from ast node
    return *l < *r;
  }

  Value_Ptr Eval::op_numbers(enum Sass_OP op, const Number& l, const Number& r, struct Sass_Inspect_Options opt, ParserState* pstate)
  {
    double lv = l.value();
    double rv = r.value();
    if (op == Sass_OP::DIV && rv == 0) {
      // XXX: this is never hit via spec tests
      return SASS_MEMORY_NEW(String_Quoted, pstate ? *pstate : l.pstate(), lv ? "Infinity" : "NaN");
    }
    if (op == Sass_OP::MOD && !rv) {
      // XXX: this is never hit via spec tests
      throw Exception::ZeroDivisionError(l, r);
    }

    Number tmp(&r); // copy
    bool strict = op != Sass_OP::MUL && op != Sass_OP::DIV;
    tmp.normalize(l.find_convertible_unit(), strict);
    std::string l_unit(l.unit());
    std::string r_unit(tmp.unit());
    Number_Obj v = SASS_MEMORY_COPY(&l); // copy
    v->pstate(pstate ? *pstate : l.pstate());
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
      v->value(ops[op](lv, r.value() * r.convert_factor(l)));
      // v->normalize();
      return v.detach();

      v->value(ops[op](lv, tmp.value()));
    }
    v->normalize();
    return v.detach();
  }

  Value_Ptr Eval::op_number_color(enum Sass_OP op, const Number& l, const Color& r, struct Sass_Inspect_Options opt, ParserState* pstate)
  {
    double lv = l.value();
    switch (op) {
      case Sass_OP::ADD:
      case Sass_OP::MUL: {
        return SASS_MEMORY_NEW(Color,
                               pstate ? *pstate : l.pstate(),
                               ops[op](lv, r.r()),
                               ops[op](lv, r.g()),
                               ops[op](lv, r.b()),
                               r.a());
      } break;
      case Sass_OP::SUB:
      case Sass_OP::DIV: {
        std::string sep(op == Sass_OP::SUB ? "-" : "/");
        std::string color(r.to_string(opt));
        return SASS_MEMORY_NEW(String_Quoted,
                               pstate ? *pstate : l.pstate(),
                               l.to_string(opt)
                               + sep
                               + color);
      } break;
      case Sass_OP::MOD: {
        throw Exception::UndefinedOperation(&l, &r, sass_op_to_name(op));
      } break;
      default: break; // caller should ensure that we don't get here
    }
    // unreachable
    return NULL;
  }

  Value_Ptr Eval::op_color_number(enum Sass_OP op, const Color& l, const Number& r, struct Sass_Inspect_Options opt, ParserState* pstate)
  {
    double rv = r.value();
    if (op == Sass_OP::DIV && !rv) {
      // comparison of Fixnum with Float failed?
      throw Exception::ZeroDivisionError(l, r);
    }
    return SASS_MEMORY_NEW(Color,
                           pstate ? *pstate : l.pstate(),
                           ops[op](l.r(), rv),
                           ops[op](l.g(), rv),
                           ops[op](l.b(), rv),
                           l.a());
  }

  Value_Ptr Eval::op_colors(enum Sass_OP op, const Color& l, const Color& r, struct Sass_Inspect_Options opt, ParserState* pstate)
  {
    if (l.a() != r.a()) {
      throw Exception::AlphaChannelsNotEqual(&l, &r, "+");
    }
    if (op == Sass_OP::DIV && (!r.r() || !r.g() ||!r.b())) {
      // comparison of Fixnum with Float failed?
      throw Exception::ZeroDivisionError(l, r);
    }
    return SASS_MEMORY_NEW(Color,
                           pstate ? *pstate : l.pstate(),
                           ops[op](l.r(), r.r()),
                           ops[op](l.g(), r.g()),
                           ops[op](l.b(), r.b()),
                           l.a());
  }

  Value_Ptr Eval::op_strings(Sass::Operand operand, Value& lhs, Value& rhs, struct Sass_Inspect_Options opt, ParserState* pstate, bool delayed)
  {
    Expression::Concrete_Type ltype = lhs.concrete_type();
    Expression::Concrete_Type rtype = rhs.concrete_type();
    enum Sass_OP op = operand.operand;

    String_Quoted_Ptr lqstr = Cast<String_Quoted>(&lhs);
    String_Quoted_Ptr rqstr = Cast<String_Quoted>(&rhs);

    std::string lstr(lqstr ? lqstr->value() : lhs.to_string(opt));
    std::string rstr(rqstr ? rqstr->value() : rhs.to_string(opt));

    if (ltype == Expression::NULL_VAL) throw Exception::InvalidNullOperation(&lhs, &rhs, sass_op_to_name(op));
    if (rtype == Expression::NULL_VAL) throw Exception::InvalidNullOperation(&lhs, &rhs, sass_op_to_name(op));
    if (op == Sass_OP::MOD) throw Exception::UndefinedOperation(&lhs, &rhs, sass_op_to_name(op));
    if (op == Sass_OP::MUL) throw Exception::UndefinedOperation(&lhs, &rhs, sass_op_to_name(op));
    std::string sep;
    switch (op) {
      case Sass_OP::SUB: sep = "-"; break;
      case Sass_OP::DIV: sep = "/"; break;
      case Sass_OP::MUL: sep = "*"; break;
      case Sass_OP::MOD: sep = "%"; break;
      case Sass_OP::EQ:  sep = "=="; break;
      case Sass_OP::NEQ:  sep = "!="; break;
      case Sass_OP::LT:  sep = "<"; break;
      case Sass_OP::GT:  sep = ">"; break;
      case Sass_OP::LTE:  sep = "<="; break;
      case Sass_OP::GTE:  sep = ">="; break;
      default:                      break;
    }

    if ( (sep == "") /* &&
         (sep != "/" || !rqstr || !rqstr->quote_mark()) */
    ) {
      // create a new string that might be quoted on output (but do not unquote what we pass)
      return SASS_MEMORY_NEW(String_Quoted, pstate ? *pstate : lhs.pstate(), lstr + rstr, 0, false, true);
    }

    if (sep != "" && !delayed) {
      if (operand.ws_before) sep = " " + sep;
      if (operand.ws_after) sep = sep + " ";
    }

    if (op == Sass_OP::SUB || op == Sass_OP::DIV) {
      if (lqstr && lqstr->quote_mark()) lstr = quote(lstr);
      if (rqstr && rqstr->quote_mark()) rstr = quote(rstr);
    }

    return SASS_MEMORY_NEW(String_Constant, pstate ? *pstate : lhs.pstate(), lstr + sep + rstr);
  }

  Expression_Ptr cval_to_astnode(union Sass_Value* v, Backtrace* backtrace, ParserState pstate)
  {
    using std::strlen;
    using std::strcpy;
    Expression_Ptr e = NULL;
    switch (sass_value_get_tag(v)) {
      case SASS_BOOLEAN: {
        e = SASS_MEMORY_NEW(Boolean, pstate, !!sass_boolean_get_value(v));
      } break;
      case SASS_NUMBER: {
        e = SASS_MEMORY_NEW(Number, pstate, sass_number_get_value(v), sass_number_get_unit(v));
      } break;
      case SASS_COLOR: {
        e = SASS_MEMORY_NEW(Color, pstate, sass_color_get_r(v), sass_color_get_g(v), sass_color_get_b(v), sass_color_get_a(v));
      } break;
      case SASS_STRING: {
        if (sass_string_is_quoted(v))
          e = SASS_MEMORY_NEW(String_Quoted, pstate, sass_string_get_value(v));
        else {
          e = SASS_MEMORY_NEW(String_Constant, pstate, sass_string_get_value(v));
        }
      } break;
      case SASS_LIST: {
        List_Ptr l = SASS_MEMORY_NEW(List, pstate, sass_list_get_length(v), sass_list_get_separator(v));
        for (size_t i = 0, L = sass_list_get_length(v); i < L; ++i) {
          l->append(cval_to_astnode(sass_list_get_value(v, i), backtrace, pstate));
        }
        l->is_bracketed(sass_list_get_is_bracketed(v));
        e = l;
      } break;
      case SASS_MAP: {
        Map_Ptr m = SASS_MEMORY_NEW(Map, pstate);
        for (size_t i = 0, L = sass_map_get_length(v); i < L; ++i) {
          *m << std::make_pair(
            cval_to_astnode(sass_map_get_key(v, i), backtrace, pstate),
            cval_to_astnode(sass_map_get_value(v, i), backtrace, pstate));
        }
        e = m;
      } break;
      case SASS_NULL: {
        e = SASS_MEMORY_NEW(Null, pstate);
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

  Selector_List_Ptr Eval::operator()(Selector_List_Ptr s)
  {
    std::vector<Selector_List_Obj> rv;
    Selector_List_Obj sl = SASS_MEMORY_NEW(Selector_List, s->pstate());
    sl->is_optional(s->is_optional());
    sl->media_block(s->media_block());
    sl->is_optional(s->is_optional());
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
          sl->append((*rv[i])[round]);
          abort = false;
        }
      }
      if (abort) {
        round = std::string::npos;
      } else {
        ++ round;
      }

    }
    return sl.detach();
  }


  Selector_List_Ptr Eval::operator()(Complex_Selector_Ptr s)
  {
    bool implicit_parent = !exp.old_at_root_without_rule;
    return s->resolve_parent_refs(ctx, exp.selector_stack, implicit_parent);
  }

  // XXX: this is never hit via spec tests
  Attribute_Selector_Ptr Eval::operator()(Attribute_Selector_Ptr s)
  {
    String_Obj attr = s->value();
    if (attr) { attr = static_cast<String_Ptr>(attr->perform(this)); }
    Attribute_Selector_Ptr ss = SASS_MEMORY_COPY(s);
    ss->value(attr);
    return ss;
  }

  Selector_List_Ptr Eval::operator()(Selector_Schema_Ptr s)
  {
    // the parser will look for a brace to end the selector
    Expression_Obj sel = s->contents()->perform(this);
    std::string result_str(sel->to_string(ctx.c_options));
    result_str = unquote(Util::rtrim(result_str));
    Parser p = Parser::from_c_str(result_str.c_str(), ctx, s->pstate());
    p.last_media_block = s->media_block();
    // a selector schema may or may not connect to parent?
    bool chroot = s->connect_parent() == false;
    Selector_List_Obj sl = p.parse_selector_list(chroot);
    return operator()(sl);
  }

  Expression_Ptr Eval::operator()(Parent_Selector_Ptr p)
  {
    if (Selector_List_Obj pr = selector()) {
      exp.selector_stack.pop_back();
      Selector_List_Obj rv = operator()(pr);
      exp.selector_stack.push_back(rv);
      return rv.detach();
    } else {
      return SASS_MEMORY_NEW(Null, p->pstate());
    }
  }

}
