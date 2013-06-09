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
      env = old_env;
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