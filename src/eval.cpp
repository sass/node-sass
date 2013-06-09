#include "eval.hpp"
#include "bind.hpp"
#include "to_string.hpp"
#include "context.hpp"
#include "ast.hpp"
#include "prelexer.hpp"
#include <cstdlib>

#include <iostream>

namespace Sass {
  using namespace std;

  Eval::Eval(Context& ctx, Env* env) : ctx(ctx), env(env) { }
  Eval::~Eval() { }

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
      error("lower bound of `@for` directive must be numeric", low->path(), low->line());
    }
    Expression* high = f->upper_bound()->perform(this);
    if (high->concrete_type() != Expression::NUMBER) {
      error("upper bound of `@for` directive must be numeric", high->path(), high->line());
    }
    double lo = static_cast<Number*>(low)->value();
    double hi = static_cast<Number*>(high)->value();
    if (f->is_inclusive()) ++hi;
    Env new_env;
    new_env[variable] = new (ctx.mem) Number(low->path(), low->line(), lo);
    new_env.link(env);
    env = &new_env;
    Block* body = f->block();
    Expression* val = 0;
    for (size_t i = lo;
         i < hi;
         (*env)[variable] = new (ctx.mem) Number(low->path(), low->line(), ++i)) {
      val = body->perform(this);
      if (val) break;
    }
    env = new_env.parent();
    return val;
  }

  Expression* Eval::operator()(Each* e)
  {
    string variable(e->variable());
    Expression* expr = e->list()->perform(this);
    List* list = 0;
    if (expr->concrete_type() != Expression::LIST) {
      list = new (ctx.mem) List(expr->path(), expr->line(), 1, List::COMMA);
      *list << expr;
    }
    else {
      list = static_cast<List*>(expr);
    }
    Env new_env;
    new_env[variable] = 0;
    new_env.link(env);
    env = &new_env;
    Block* body = e->block();
    Expression* val = 0;
    for (size_t i = 0, L = list->length(); i < L; ++i) {
      (*env)[variable] = (*list)[i];
      val = body->perform(this);
      if (val) break;
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

  Expression* Eval::operator()(List* l)
  {
    List* ll = new (ctx.mem) List(l->path(),
                                  l->line(),
                                  l->length(),
                                  l->separator(),
                                  l->is_arglist());
    for (size_t i = 0, L = l->length(); i < L; ++i) {
      *ll << (*l)[i]->perform(this);
    }
    return ll;
  }

  Expression* Eval::operator()(Unary_Expression* u)
  {
    Expression* operand = u->operand()->perform(this);
    if (operand->concrete_type() == Expression::NUMBER) {
      Number* result = new (ctx.mem) Number(*static_cast<Number*>(operand));
      result->value(u->type() == Unary_Expression::MINUS
                    ? -result->value()
                    :  result->value());
      return result;
    }
    else {
      Unary_Expression* result = new (ctx.mem) Unary_Expression(u->path(),
                                                                u->line(),
                                                                u->type(),
                                                                operand);
      return result;
    }
    // unreachable
    return u;
  }

  Expression* Eval::operator()(Function_Call* c)
  {
    Arguments* args = static_cast<Arguments*>(c->arguments()->perform(this));
    string full_name(c->name() + "[f]");

    // if it doesn't exist, just pass it through as a literal
    if (!env->has(full_name)) {
      return new (ctx.mem) Function_Call(c->path(),
                                         c->line(),
                                         c->name(),
                                         args);
    }

    Expression*     result = c;
    Definition*     def    = static_cast<Definition*>((*env)[full_name]);
    Block*          body   = def->block();
    Native_Function func   = def->native_function();

    // if it's user-defined, bind the args and eval the body
    if (body) {
      Parameters* params = def->parameters();
      Env new_env;
      bind("function " + c->name(), params, args, ctx, &new_env);
      new_env.link(def->environment());
      Env* old_env = env;
      env = &new_env;
      Expression* result = body->perform(this);
      if (!result) {
        error(string("function ") + c->name() + " did not return a value", c->path(), c->line());
      }
      env = old_env;
      return result;
    }
    // if it's native, invoke the underlying CPP function
    else if (func) {
      // do stuff
    }

    return result;
  }

  Expression* Eval::operator()(Function_Call_Schema* s)
  {
    Expression* evaluated_name = s->name()->perform(this);
    Expression* evaluated_args = s->arguments()->perform(this);
    String_Schema* ss = new (ctx.mem) String_Schema(s->path(), s->line(), 2);
    (*ss) << evaluated_name << evaluated_args;
    return ss->perform(this);
  }

  Expression* Eval::operator()(Variable* v)
  {
    string name(v->name());
    if (env->has(name)) return static_cast<Expression*>((*env)[name]);
    else error("unbound variable " + v->name(), v->path(), v->line());
  }

  Expression* Eval::operator()(Textual* t)
  {
    using Prelexer::number;
    Expression* result;
    switch (t->type())
    {
      case Textual::NUMBER:
        result = new (ctx.mem) Number(t->path(),
                                      t->line(),
                                      atof(t->value().c_str()));
        break;
      case Textual::PERCENTAGE:
        result = new (ctx.mem) Number(t->path(),
                                      t->line(),
                                      atof(t->value().c_str()),
                                      "%");
        break;
      case Textual::DIMENSION:
        result = new (ctx.mem) Number(t->path(),
                                      t->line(),
                                      atof(t->value().c_str()),
                                      Token(number(t->value().c_str())));
        break;
      case Textual::HEX: {
        string hext(t->value().substr(1)); // chop off the '#'
        if (hext.length() == 6) {
          result = new (ctx.mem) Color(t->path(),
                                       t->line(),
                                       static_cast<double>(strtol(hext.substr(0,2).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(hext.substr(2,4).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(hext.substr(4,6).c_str(), NULL, 16)));
        }
        else {
          result = new (ctx.mem) Color(t->path(),
                                       t->line(),
                                       static_cast<double>(strtol(string(2,hext[0]).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(string(2,hext[1]).c_str(), NULL, 16)),
                                       static_cast<double>(strtol(string(2,hext[2]).c_str(), NULL, 16)));
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

  Expression* Eval::operator()(String_Schema* s)
  {
    string acc;
    To_String to_string;
    for (size_t i = 0, L = s->length(); i < L; ++i) {
      acc += (*s)[i]->perform(this)->perform(&to_string);
    }
    return new (ctx.mem) String_Constant(s->path(),
                                         s->line(),
                                         acc);
  }

  Expression* Eval::operator()(String_Constant* s)
  {
    return s;
  }

  Expression* Eval::operator()(Media_Query* q)
  {
    String* t = q->media_type();
    t = static_cast<String*>(t ? t->perform(this) : 0);
    Media_Query* qq = new (ctx.mem) Media_Query(q->path(),
                                                q->line(),
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
    String* feature = e->feature();
    feature = static_cast<String*>(feature ? feature->perform(this) : 0);
    Expression* value = e->value();
    value = (value ? value->perform(this) : 0);
    return new (ctx.mem) Media_Query_Expression(e->path(),
                                                e->line(),
                                                feature,
                                                value);
  }

  Expression* Eval::operator()(Argument* a)
  {
    Expression* val = a->value()->perform(this);
    if (a->is_rest_argument() && (val->concrete_type() != Expression::LIST)) {
      List* wrapper = new (ctx.mem) List(val->path(),
                                         val->line(),
                                         0,
                                         List::COMMA,
                                         true);
      *wrapper << val;
      val = wrapper;
    }
    return new (ctx.mem) Argument(a->path(),
                                  a->line(),
                                  val,
                                  a->name(),
                                  a->is_rest_argument());
  }

  Expression* Eval::operator()(Arguments* a)
  {
    Arguments* aa = new (ctx.mem) Arguments(a->path(), a->line());
    for (size_t i = 0, L = a->length(); i < L; ++i) {
      *aa << static_cast<Argument*>((*a)[i]->perform(this));
    }
    return aa;
  }

  inline Expression* Eval::fallback_impl(AST_Node* n)
  {
    return static_cast<Expression*>(n);
  }
}