#ifndef SASS_CHECK_NESTING_H
#define SASS_CHECK_NESTING_H

#include "ast.hpp"
#include "operation.hpp"

namespace Sass {

  typedef Environment<AST_Node*> Env;

  class CheckNesting : public Operation_CRTP<Statement*, CheckNesting> {

    std::vector<Statement*>  parents;
    Statement*               parent;
    Definition*              current_mixin_definition;

    Statement* fallback_impl(Statement*);
    Statement* before(Statement*);
    Statement* visit_children(Statement*);

  public:
    CheckNesting();
    ~CheckNesting() { }

    Statement* operator()(Block*);
    Statement* operator()(Definition*);

    template <typename U>
    Statement* fallback(U x) {
        return fallback_impl(this->before(dynamic_cast<Statement*>(x)));
    }

  private:
    void invalid_content_parent(Statement*);
    void invalid_charset_parent(Statement*);
    void invalid_extend_parent(Statement*);
    // void invalid_import_parent(Statement*);
    void invalid_mixin_definition_parent(Statement*);
    void invalid_function_parent(Statement*);

    void invalid_function_child(Statement*);
    void invalid_prop_child(Statement*);
    void invalid_prop_parent(Statement*);
    void invalid_return_parent(Statement*);

    bool is_transparent_parent(Statement*, Statement*);

    bool should_visit(Statement*);

    bool is_charset(Statement*);
    bool is_mixin(Statement*);
    bool is_function(Statement*);
    bool is_root_node(Statement*);
    bool is_at_root_node(Statement*);
    bool is_directive_node(Statement*);
  };

}

#endif
