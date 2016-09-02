#include "sass.hpp"
#include <vector>

#include "check_nesting.hpp"

namespace Sass {

  CheckNesting::CheckNesting()
  : parents(std::vector<Statement*>()),
    parent(0),
    current_mixin_definition(0)
  { }

  Statement* CheckNesting::before(Statement* s) {
      if (this->should_visit(s)) return s;
      return 0;
  }

  Statement* CheckNesting::visit_children(Statement* parent) {

    Statement* old_parent = this->parent;

    if (dynamic_cast<At_Root_Block*>(parent)) {
      std::vector<Statement*> old_parents = this->parents;
      std::vector<Statement*> new_parents;

      for (size_t i = 0, L = this->parents.size(); i < L; i++) {
        Statement* p = this->parents.at(i);
        if (!dynamic_cast<At_Root_Block*>(parent)->exclude_node(p)) {
          new_parents.push_back(p);
        }
      }
      this->parents = new_parents;

      for (size_t i = this->parents.size(); i > 0; i--) {
        Statement* p = 0;
        Statement* gp = 0;
        if (i > 0) p = this->parents.at(i - 1);
        if (i > 1) gp = this->parents.at(i - 2);

        if (!this->is_transparent_parent(p, gp)) {
          this->parent = p;
          break;
        }
      }

      At_Root_Block* ar = dynamic_cast<At_Root_Block*>(parent);
      Statement* ret = this->visit_children(ar->block());

      this->parent = old_parent;
      this->parents = old_parents;

      return ret;
    }


    if (!this->is_transparent_parent(parent, old_parent)) {
      this->parent = parent;
    }

    this->parents.push_back(parent);

    Block* b = dynamic_cast<Block*>(parent);

    if (!b) {
      if (Has_Block* bb = dynamic_cast<Has_Block*>(parent)) {
        b = bb->block();
      }
    }

    if (b) {
      for (auto n : *b) {
        n->perform(this);
      }
    }

    this->parent = old_parent;
    this->parents.pop_back();

    return b;
  }


  Statement* CheckNesting::operator()(Block* b)
  {
    return this->visit_children(b);
  }

  Statement* CheckNesting::operator()(Definition* n)
  {
    if (!is_mixin(n)) return n;

    Definition* old_mixin_definition = this->current_mixin_definition;
    this->current_mixin_definition = n;

    visit_children(n);

    this->current_mixin_definition = old_mixin_definition;

    return n;
  }

  Statement* CheckNesting::fallback_impl(Statement* s)
  {
    if (dynamic_cast<Block*>(s) || dynamic_cast<Has_Block*>(s)) {
      return visit_children(s);
    }
    return s;
  }

  bool CheckNesting::should_visit(Statement* node)
  {
    if (!this->parent) return true;

    if (dynamic_cast<Content*>(node))
    { this->invalid_content_parent(this->parent); }

    if (is_charset(node))
    { this->invalid_charset_parent(this->parent); }

    if (dynamic_cast<Extension*>(node))
    { this->invalid_extend_parent(this->parent); }

    // if (dynamic_cast<Import*>(node))
    // { this->invalid_import_parent(this->parent); }

    if (this->is_mixin(node))
    { this->invalid_mixin_definition_parent(this->parent); }

    if (this->is_function(node))
    { this->invalid_function_parent(this->parent); }

    if (this->is_function(this->parent))
    { this->invalid_function_child(node); }

    if (dynamic_cast<Declaration*>(node))
    { this->invalid_prop_parent(this->parent); }

    if (
      dynamic_cast<Declaration*>(this->parent)
    ) { this->invalid_prop_child(node); }

    if (dynamic_cast<Return*>(node))
    { this->invalid_return_parent(this->parent); }

    return true;
  }

  void CheckNesting::invalid_content_parent(Statement* parent)
  {
    if (!this->current_mixin_definition) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "@content may only be used within a mixin."
      );
    }
  }

  void CheckNesting::invalid_charset_parent(Statement* parent)
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

  void CheckNesting::invalid_extend_parent(Statement* parent)
  {
    if (!(
        dynamic_cast<Ruleset*>(parent) ||
        dynamic_cast<Mixin_Call*>(parent) ||
        is_mixin(parent)
    )) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "Extend directives may only be used within rules."
      );
    }
  }

  // void CheckNesting::invalid_import_parent(Statement* parent)
  // {
  //   for (auto pp : this->parents) {
  //     if (
  //         dynamic_cast<Each*>(pp) ||
  //         dynamic_cast<For*>(pp) ||
  //         dynamic_cast<If*>(pp) ||
  //         dynamic_cast<While*>(pp) ||
  //         dynamic_cast<Trace*>(pp) ||
  //         dynamic_cast<Mixin_Call*>(pp) ||
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

  void CheckNesting::invalid_mixin_definition_parent(Statement* parent)
  {
    for (auto pp : this->parents) {
      if (
          dynamic_cast<Each*>(pp) ||
          dynamic_cast<For*>(pp) ||
          dynamic_cast<If*>(pp) ||
          dynamic_cast<While*>(pp) ||
          dynamic_cast<Trace*>(pp) ||
          dynamic_cast<Mixin_Call*>(pp) ||
          is_mixin(pp)
      ) {
        throw Exception::InvalidSass(
          parent->pstate(),
          "Mixins may not be defined within control directives or other mixins."
        );
      }
    }
  }

  void CheckNesting::invalid_function_parent(Statement* parent)
  {
    for (auto pp : this->parents) {
      if (
          dynamic_cast<Each*>(pp) ||
          dynamic_cast<For*>(pp) ||
          dynamic_cast<If*>(pp) ||
          dynamic_cast<While*>(pp) ||
          dynamic_cast<Trace*>(pp) ||
          dynamic_cast<Mixin_Call*>(pp) ||
          is_mixin(pp)
      ) {
        throw Exception::InvalidSass(
          parent->pstate(),
          "Functions may not be defined within control directives or other mixins."
        );
      }
    }
  }

  void CheckNesting::invalid_function_child(Statement* child)
  {
    if (!(
        dynamic_cast<Each*>(child) ||
        dynamic_cast<For*>(child) ||
        dynamic_cast<If*>(child) ||
        dynamic_cast<While*>(child) ||
        dynamic_cast<Trace*>(child) ||
        dynamic_cast<Comment*>(child) ||
        dynamic_cast<Debug*>(child) ||
        dynamic_cast<Return*>(child) ||
        dynamic_cast<Variable*>(child) ||
        dynamic_cast<Warning*>(child) ||
        dynamic_cast<Error*>(child)
    )) {
      throw Exception::InvalidSass(
        child->pstate(),
        "Functions can only contain variable declarations and control directives."
      );
    }
  }

  void CheckNesting::invalid_prop_child(Statement* child)
  {
    if (!(
        dynamic_cast<Each*>(child) ||
        dynamic_cast<For*>(child) ||
        dynamic_cast<If*>(child) ||
        dynamic_cast<While*>(child) ||
        dynamic_cast<Trace*>(child) ||
        dynamic_cast<Comment*>(child) ||
        dynamic_cast<Declaration*>(child) ||
        dynamic_cast<Mixin_Call*>(child)
    )) {
      throw Exception::InvalidSass(
        child->pstate(),
        "Illegal nesting: Only properties may be nested beneath properties."
      );
    }
  }

  void CheckNesting::invalid_prop_parent(Statement* parent)
  {
    if (!(
        is_mixin(parent) ||
        is_directive_node(parent) ||
        dynamic_cast<Ruleset*>(parent) ||
        dynamic_cast<Keyframe_Rule*>(parent) ||
        dynamic_cast<Declaration*>(parent) ||
        dynamic_cast<Mixin_Call*>(parent)
    )) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "Properties are only allowed within rules, directives, mixin includes, or other properties."
      );
    }
  }

  void CheckNesting::invalid_return_parent(Statement* parent)
  {
    if (!this->is_function(parent)) {
      throw Exception::InvalidSass(
        parent->pstate(),
        "@return may only be used within a function."
      );
    }
  }

  bool CheckNesting::is_transparent_parent(Statement* parent, Statement* grandparent)
  {
    bool parent_bubbles = parent && parent->bubbles();

    bool valid_bubble_node = parent_bubbles &&
                             !is_root_node(grandparent) &&
                             !is_at_root_node(grandparent);

    return dynamic_cast<Import*>(parent) ||
           dynamic_cast<Each*>(parent) ||
           dynamic_cast<For*>(parent) ||
           dynamic_cast<If*>(parent) ||
           dynamic_cast<While*>(parent) ||
           dynamic_cast<Trace*>(parent) ||
           valid_bubble_node;
  }

  bool CheckNesting::is_charset(Statement* n)
  {
    Directive* d = dynamic_cast<Directive*>(n);
    return d && d->keyword() == "charset";
  }

  bool CheckNesting::is_mixin(Statement* n)
  {
    Definition* def = dynamic_cast<Definition*>(n);
    return def && def->type() == Definition::MIXIN;
  }

  bool CheckNesting::is_function(Statement* n)
  {
    Definition* def = dynamic_cast<Definition*>(n);
    return def && def->type() == Definition::FUNCTION;
  }

  bool CheckNesting::is_root_node(Statement* n)
  {
    if (dynamic_cast<Ruleset*>(n)) return false;

    Block* b = dynamic_cast<Block*>(n);
    return b && b->is_root();
  }

  bool CheckNesting::is_at_root_node(Statement* n)
  {
    return dynamic_cast<At_Root_Block*>(n) != NULL;
  }

  bool CheckNesting::is_directive_node(Statement* n)
  {
    return dynamic_cast<Directive*>(n) ||
           dynamic_cast<Import*>(n) ||
           dynamic_cast<Media_Block*>(n) ||
           dynamic_cast<Supports_Block*>(n);
  }
}
