#include "sass.hpp"
#include <vector>

#include "check_nesting.hpp"
#include "context.hpp"
// #include "debugger.hpp"

namespace Sass {

  CheckNesting::CheckNesting(Context& ctx)
  : ctx(ctx),
    parent_stack(std::vector<AST_Node*>())
  { }

  AST_Node* CheckNesting::parent()
  {
    if (parent_stack.size() > 0)
      return parent_stack.back();
    return 0;
  }

  Statement* CheckNesting::operator()(Block* b)
  {
    parent_stack.push_back(b);

    for (auto n : *b) {
      n->perform(this);
    }

    parent_stack.pop_back();
    return b;
  }

  Statement* CheckNesting::operator()(Declaration* d)
  {
    if (!is_valid_prop_parent(parent())) {
      throw Exception::InvalidSass(d->pstate(), "Properties are only allowed "
        "within rules, directives, mixin includes, or other properties.");
    }
    return static_cast<Statement*>(d);
  }

  Statement* CheckNesting::fallback_impl(AST_Node* n)
  {
    return static_cast<Statement*>(n);
  }

  bool CheckNesting::is_valid_prop_parent(AST_Node* p) 
  {
    if (Definition* def = dynamic_cast<Definition*>(p)) {
      return def->type() == Definition::MIXIN;
    }

    return dynamic_cast<Ruleset*>(p) ||
           dynamic_cast<Keyframe_Rule*>(p) ||
           dynamic_cast<Propset*>(p) ||
           dynamic_cast<Directive*>(p) ||
           dynamic_cast<Mixin_Call*>(p);
  }
}
