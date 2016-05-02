#ifndef SASS_CHECK_NESTING_H
#define SASS_CHECK_NESTING_H

#include "ast.hpp"
#include "operation.hpp"

namespace Sass {

  typedef Environment<AST_Node*> Env;

  class CheckNesting : public Operation_CRTP<Statement*, CheckNesting> {

    std::vector<AST_Node*>   parent_stack;

    AST_Node* parent();

    Statement* fallback_impl(AST_Node* n);

  public:
    CheckNesting();
    ~CheckNesting() { }

    Statement* operator()(Block*);
    Statement* operator()(Declaration*);

    template <typename U>
    Statement* fallback(U x) { return fallback_impl(x); }

    bool is_valid_prop_parent(AST_Node*);
  };

}

#endif
