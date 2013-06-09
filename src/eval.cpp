#include "eval.hpp"
#include "context.hpp"
#include "ast.hpp"
#include "prelexer.hpp"
#include <cstdlib>

#include <typeinfo>

namespace Sass {
  using namespace std;

  Eval::Eval(Context& ctx, Env* env) : ctx(ctx), env(env) { }
  Eval::~Eval() { }

  // inline Eval* Eval::with(Env* e)
  // {
  //   env = e;
  //   return this;
  // }

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

  Expression* Eval::operator()(Variable* v)
  {
    string name(v->name());
    if (env->has(name)) return static_cast<Expression*>((*env)[name]);
    else fallback_impl(v); // TODO: error
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
        result = new (ctx.mem) Percentage(t->path(),
                                          t->line(),
                                          atof(t->value().c_str()));
        break;
      case Textual::DIMENSION:
        result = new (ctx.mem) Dimension(t->path(),
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

  Expression* Eval::operator()(String_Schema* s)
  {

  }

  Expression* Eval::operator()(String_Constant* s)
  {
    return s;
  }

  Expression* Eval::operator()(Argument* a)
  {
    Expression* val = a->value()->perform(this);
    if (a->is_rest_argument() && (typeid(*val) != typeid(List))) {
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
    cerr << "Fallback invoked on " << typeid(*n).name() << endl;
    return new (ctx.mem) String_Constant("", 0, "[tbd]");
  }
}