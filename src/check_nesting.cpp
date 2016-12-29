#include "sass.hpp"
#include <vector>

#include "check_nesting.hpp"

namespace Sass {

  CheckNesting::CheckNesting()
  : parents(std::vector<Statement_Ptr>()),
    parent(0),
    current_mixin_definition(0)
  { }

  Statement_Ptr CheckNesting::visit_children(Statement_Ptr parent)
  {
    Statement_Ptr old_parent = this->parent;

    if (At_Root_Block_Ptr root = SASS_MEMORY_CAST_PTR(At_Root_Block, parent)) {
      std::vector<Statement_Ptr> old_parents = this->parents;
      std::vector<Statement_Ptr> new_parents;

      for (size_t i = 0, L = this->parents.size(); i < L; i++) {
        Statement_Ptr p = this->parents.at(i);
        if (!root->exclude_node(p)) {
          new_parents.push_back(p);
        }
      }
      this->parents = new_parents;

      for (size_t i = this->parents.size(); i > 0; i--) {
        Statement_Ptr p = 0;
        Statement_Ptr gp = 0;
        if (i > 0) p = this->parents.at(i - 1);
        if (i > 1) gp = this->parents.at(i - 2);

        if (!this->is_transparent_parent(p, gp)) {
          this->parent = p;
          break;
        }
      }

      At_Root_Block_Ptr ar = SASS_MEMORY_CAST_PTR(At_Root_Block, parent);
      Statement_Ptr ret = this->visit_children(&ar->block());

      this->parent = old_parent;
      this->parents = old_parents;

      return ret;
    }

    if (!this->is_transparent_parent(parent, old_parent)) {
      this->parent = parent;
    }

    this->parents.push_back(parent);

    Block_Ptr b = SASS_MEMORY_CAST_PTR(Block, parent);

    if (!b) {
      if (Has_Block_Ptr bb = SASS_MEMORY_CAST(Has_Block, *parent)) {
        b = &bb->block();
      }
    }

    if (b) {
      for (auto n : b->elements()) {
        n->perform(this);
      }
    }
    this->parent = old_parent;
    this->parents.pop_back();

    return b;
  }


  Statement_Ptr CheckNesting::operator()(Block_Ptr b)
  {
    return this->visit_children(b);
  }

  Statement_Ptr CheckNesting::operator()(Definition_Ptr n)
  {
    if (!this->should_visit(n)) return NULL;
    if (!is_mixin(n)) {
      visit_children(n);
      return n;
    }

    Definition_Ptr old_mixin_definition = this->current_mixin_definition;
    this->current_mixin_definition = n;

    visit_children(n);

    this->current_mixin_definition = old_mixin_definition;

    return n;
  }

  Statement_Ptr CheckNesting::fallback_impl(Statement_Ptr s)
  {
    Block_Ptr b1 = SASS_MEMORY_CAST_PTR(Block, s);
    Has_Block_Ptr b2 = SASS_MEMORY_CAST_PTR(Has_Block, s);
    return b1 || b2 ? visit_children(s) : s;
  }

  bool CheckNesting::should_visit(Statement_Ptr node)
  {
    if (!this->parent) return true;

    if (SASS_MEMORY_CAST_PTR(Content, node))
    { this->invalid_content_parent(this->parent); }

    if (is_charset(node))
    { this->invalid_charset_parent(this->parent); }

    if (SASS_MEMORY_CAST_PTR(Extension, node))
    { this->invalid_extend_parent(this->parent); }

    // if (SASS_MEMORY_CAST(Import, node))
    // { this->invalid_import_parent(this->parent); }

    if (this->is_mixin(node))
    { this->invalid_mixin_definition_parent(this->parent); }

    if (this->is_function(node))
    { this->invalid_function_parent(this->parent); }

    if (this->is_function(this->parent))
    { this->invalid_function_child(node); }

    if (SASS_MEMORY_CAST_PTR(Declaration, node))
    { this->invalid_prop_parent(this->parent); }

    if (SASS_MEMORY_CAST_PTR(Declaration, this->parent))
    { this->invalid_prop_child(node); }

    if (SASS_MEMORY_CAST_PTR(Return, node))
    { this->invalid_return_parent(this->parent); }

    return true;
  }

  void CheckNesting::invalid_content_parent(Statement_Ptr parent)
  {
    if (!this->current_mixin_definition) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "@content may only be used within a mixin."
      );
    }
  }

  void CheckNesting::invalid_charset_parent(Statement_Ptr parent)
  {
    if (!(
        is_root_node(parent)
    )) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "@charset may only be used at the root of a document."
      );
    }
  }

  void CheckNesting::invalid_extend_parent(Statement_Ptr parent)
  {
    if (!(
        SASS_MEMORY_CAST_PTR(Ruleset, parent) ||
        SASS_MEMORY_CAST_PTR(Mixin_Call, parent) ||
        is_mixin(parent)
    )) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "Extend directives may only be used within rules."
      );
    }
  }

  // void CheckNesting::invalid_import_parent(Statement_Ptr parent)
  // {
  //   for (auto pp : this->parents) {
  //     if (
  //         SASS_MEMORY_CAST(Each, pp) ||
  //         SASS_MEMORY_CAST(For, pp) ||
  //         SASS_MEMORY_CAST(If, pp) ||
  //         SASS_MEMORY_CAST(While, pp) ||
  //         SASS_MEMORY_CAST(Trace, pp) ||
  //         SASS_MEMORY_CAST(Mixin_Call, pp) ||
  //         is_mixin(pp)
  //     ) {
  //       throw Exception::InvalidSass(
  //         parent->pstate(),
  //         "Import directives may not be defined within control directives or other mixins."
  //       );
  //     }
  //   }

  //   if (this->is_root_node(parent)) {
  //     return;
  //   }

  //   if (false/*n.css_import?*/) {
  //     throw Exception::InvalidSass(
  //       parent->pstate(),
  //       "CSS import directives may only be used at the root of a document."
  //     );
  //   }
  // }

  void CheckNesting::invalid_mixin_definition_parent(Statement_Ptr parent)
  {
    for (Statement_Ptr pp : this->parents) {
      if (
          SASS_MEMORY_CAST_PTR(Each, pp) ||
          SASS_MEMORY_CAST_PTR(For, pp) ||
          SASS_MEMORY_CAST_PTR(If, pp) ||
          SASS_MEMORY_CAST_PTR(While, pp) ||
          SASS_MEMORY_CAST_PTR(Trace, pp) ||
          SASS_MEMORY_CAST_PTR(Mixin_Call, pp) ||
          is_mixin(pp)
      ) {
        throw Exception::InvalidSass(
          parent->pstate(),
          "Mixins may not be defined within control directives or other mixins."
        );
      }
    }
  }

  void CheckNesting::invalid_function_parent(Statement_Ptr parent)
  {
    for (Statement_Ptr pp : this->parents) {
      if (
          SASS_MEMORY_CAST_PTR(Each, pp) ||
          SASS_MEMORY_CAST_PTR(For, pp) ||
          SASS_MEMORY_CAST_PTR(If, pp) ||
          SASS_MEMORY_CAST_PTR(While, pp) ||
          SASS_MEMORY_CAST_PTR(Trace, pp) ||
          SASS_MEMORY_CAST_PTR(Mixin_Call, pp) ||
          is_mixin(pp)
      ) {
        throw Exception::InvalidSass(
          parent->pstate(),
          "Functions may not be defined within control directives or other mixins."
        );
      }
    }
  }

  void CheckNesting::invalid_function_child(Statement_Ptr child)
  {
    if (!(
        SASS_MEMORY_CAST_PTR(Each, child) ||
        SASS_MEMORY_CAST_PTR(For, child) ||
        SASS_MEMORY_CAST_PTR(If, child) ||
        SASS_MEMORY_CAST_PTR(While, child) ||
        SASS_MEMORY_CAST_PTR(Trace, child) ||
        SASS_MEMORY_CAST_PTR(Comment, child) ||
        SASS_MEMORY_CAST_PTR(Debug, child) ||
        SASS_MEMORY_CAST_PTR(Return, child) ||
        SASS_MEMORY_CAST_PTR(Variable, child) ||
        // Ruby Sass doesn't distinguish variables and assignments
        SASS_MEMORY_CAST_PTR(Assignment, child) ||
        SASS_MEMORY_CAST_PTR(Warning, child) ||
        SASS_MEMORY_CAST_PTR(Error, child)
    )) {
      throw Exception::InvalidSass(
        child->pstate(),
        "Functions can only contain variable declarations and control directives."
      );
    }
  }

  void CheckNesting::invalid_prop_child(Statement_Ptr child)
  {
    if (!(
        SASS_MEMORY_CAST_PTR(Each, child) ||
        SASS_MEMORY_CAST_PTR(For, child) ||
        SASS_MEMORY_CAST_PTR(If, child) ||
        SASS_MEMORY_CAST_PTR(While, child) ||
        SASS_MEMORY_CAST_PTR(Trace, child) ||
        SASS_MEMORY_CAST_PTR(Comment, child) ||
        SASS_MEMORY_CAST_PTR(Declaration, child) ||
        SASS_MEMORY_CAST_PTR(Mixin_Call, child)
    )) {
      throw Exception::InvalidSass(
        child->pstate(),
        "Illegal nesting: Only properties may be nested beneath properties."
      );
    }
  }

  void CheckNesting::invalid_prop_parent(Statement_Ptr parent)
  {
    if (!(
        is_mixin(parent) ||
        is_directive_node(parent) ||
        SASS_MEMORY_CAST_PTR(Ruleset, parent) ||
        SASS_MEMORY_CAST_PTR(Keyframe_Rule, parent) ||
        SASS_MEMORY_CAST_PTR(Declaration, parent) ||
        SASS_MEMORY_CAST_PTR(Mixin_Call, parent)
    )) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "Properties are only allowed within rules, directives, mixin includes, or other properties."
      );
    }
  }

  void CheckNesting::invalid_return_parent(Statement_Ptr parent)
  {
    if (!this->is_function(parent)) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "@return may only be used within a function."
      );
    }
  }

  bool CheckNesting::is_transparent_parent(Statement_Ptr parent, Statement_Ptr grandparent)
  {
    bool parent_bubbles = parent && parent->bubbles();

    bool valid_bubble_node = parent_bubbles &&
                             !is_root_node(grandparent) &&
                             !is_at_root_node(grandparent);

    return SASS_MEMORY_CAST_PTR(Import, parent) ||
           SASS_MEMORY_CAST_PTR(Each, parent) ||
           SASS_MEMORY_CAST_PTR(For, parent) ||
           SASS_MEMORY_CAST_PTR(If, parent) ||
           SASS_MEMORY_CAST_PTR(While, parent) ||
           SASS_MEMORY_CAST_PTR(Trace, parent) ||
           valid_bubble_node;
  }

  bool CheckNesting::is_charset(Statement_Ptr n)
  {
    Directive_Ptr d = SASS_MEMORY_CAST_PTR(Directive, n);
    return d && d->keyword() == "charset";
  }

  bool CheckNesting::is_mixin(Statement_Ptr n)
  {
    Definition_Ptr def = SASS_MEMORY_CAST_PTR(Definition, n);
    return def && def->type() == Definition::MIXIN;
  }

  bool CheckNesting::is_function(Statement_Ptr n)
  {
    Definition_Ptr def = SASS_MEMORY_CAST_PTR(Definition, n);
    return def && def->type() == Definition::FUNCTION;
  }

  bool CheckNesting::is_root_node(Statement_Ptr n)
  {
    if (SASS_MEMORY_CAST_PTR(Ruleset, n)) return false;

    Block_Ptr b = SASS_MEMORY_CAST_PTR(Block, n);
    return b && b->is_root();
  }

  bool CheckNesting::is_at_root_node(Statement_Ptr n)
  {
    return SASS_MEMORY_CAST_PTR(At_Root_Block, n) != NULL;
  }

  bool CheckNesting::is_directive_node(Statement_Ptr n)
  {
    return SASS_MEMORY_CAST_PTR(Directive, n) ||
           SASS_MEMORY_CAST_PTR(Import, n) ||
           SASS_MEMORY_CAST_PTR(Media_Block, n) ||
           SASS_MEMORY_CAST_PTR(Supports_Block, n);
  }
}
