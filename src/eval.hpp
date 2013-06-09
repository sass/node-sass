#define SASS_EVAL

#include <iostream>

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
#endif

namespace Sass {
  using namespace std;

  class Context;
  typedef Environment<AST_Node*> Env;

  class Eval : public Operation_CRTP<Expression*, Eval> {

    Context&          ctx;
    Env*              env;

    Expression* fallback_impl(AST_Node* n);

  public:
    Eval(Context&, Env*);
    virtual ~Eval();
    Eval* with(Env* e) // for setting the env before eval'ing an expression
    {
      env = e;
      return this;
    }
    using Operation<Expression*>::operator();

    Expression* operator()(List*);
    // Expression* operator()(Binary_Expression*);
    // Expression* operator()(Unary_Expression*);
    // Expression* operator()(Function_Call*);
    Expression* operator()(Variable*);
    Expression* operator()(Textual*);
    // Expression* operator()(Number*);
    // Expression* operator()(Percentage*);
    // Expression* operator()(Dimension*);
    // Expression* operator()(Color*);
    // Expression* operator()(Boolean*);
    Expression* operator()(String_Schema*);
    Expression* operator()(String_Constant*);
    // Expression* operator()(Media_Query*);
    // Expression* operator()(Media_Query_Expression*);
    Expression* operator()(Argument*);
    Expression* operator()(Arguments*);

    template <typename U>
    Expression* fallback(U x) { return fallback_impl(x); }

    void append_block(Block*);
  };

}