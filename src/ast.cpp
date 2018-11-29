#include "sass.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "node.hpp"
#include "eval.hpp"
#include "extend.hpp"
#include "emitter.hpp"
#include "color_maps.hpp"
#include "ast_fwd_decl.hpp"
#include <set>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

namespace Sass {

  static Null sass_null(ParserState("null"));

  bool Supports_Operator::needs_parens(Supports_Condition_Obj cond) const {
    if (Supports_Operator_Obj op = Cast<Supports_Operator>(cond)) {
      return op->operand() != operand();
    }
    return Cast<Supports_Negation>(cond) != NULL;
  }

  bool Supports_Negation::needs_parens(Supports_Condition_Obj cond) const {
    return Cast<Supports_Negation>(cond) ||
           Cast<Supports_Operator>(cond);
  }

  void Argument::set_delayed(bool delayed)
  {
    if (value_) value_->set_delayed(delayed);
    is_delayed(delayed);
  }

  void Arguments::set_delayed(bool delayed)
  {
    for (Argument_Obj arg : elements()) {
      if (arg) arg->set_delayed(delayed);
    }
    is_delayed(delayed);
  }


  bool At_Root_Query::exclude(std::string str)
  {
    bool with = feature() && unquote(feature()->to_string()).compare("with") == 0;
    List_Ptr l = static_cast<List_Ptr>(value().ptr());
    std::string v;

    if (with)
    {
      if (!l || l->length() == 0) return str.compare("rule") != 0;
      for (size_t i = 0, L = l->length(); i < L; ++i)
      {
        v = unquote((*l)[i]->to_string());
        if (v.compare("all") == 0 || v == str) return false;
      }
      return true;
    }
    else
    {
      if (!l || !l->length()) return str.compare("rule") == 0;
      for (size_t i = 0, L = l->length(); i < L; ++i)
      {
        v = unquote((*l)[i]->to_string());
        if (v.compare("all") == 0 || v == str) return true;
      }
      return false;
    }
  }

  void AST_Node::update_pstate(const ParserState& pstate)
  {
    pstate_.offset += pstate - pstate_ + pstate.offset;
  }

  Argument_Obj Arguments::get_rest_argument()
  {
    if (this->has_rest_argument()) {
      for (Argument_Obj arg : this->elements()) {
        if (arg->is_rest_argument()) {
          return arg;
        }
      }
    }
    return {};
  }

  Argument_Obj Arguments::get_keyword_argument()
  {
    if (this->has_keyword_argument()) {
      for (Argument_Obj arg : this->elements()) {
        if (arg->is_keyword_argument()) {
          return arg;
        }
      }
    }
    return {};
  }

  void Arguments::adjust_after_pushing(Argument_Obj a)
  {
    if (!a->name().empty()) {
      if (has_keyword_argument()) {
        coreError("named arguments must precede variable-length argument", a->pstate());
      }
      has_named_arguments(true);
    }
    else if (a->is_rest_argument()) {
      if (has_rest_argument()) {
        coreError("functions and mixins may only be called with one variable-length argument", a->pstate());
      }
      if (has_keyword_argument_) {
        coreError("only keyword arguments may follow variable arguments", a->pstate());
      }
      has_rest_argument(true);
    }
    else if (a->is_keyword_argument()) {
      if (has_keyword_argument()) {
        coreError("functions and mixins may only be called with one keyword argument", a->pstate());
      }
      has_keyword_argument(true);
    }
    else {
      if (has_rest_argument()) {
        coreError("ordinal arguments must precede variable-length arguments", a->pstate());
      }
      if (has_named_arguments()) {
        coreError("ordinal arguments must precede named arguments", a->pstate());
      }
    }
  }

  bool Ruleset::is_invisible() const {
    if (Selector_List_Ptr sl = Cast<Selector_List>(selector())) {
      for (size_t i = 0, L = sl->length(); i < L; ++i)
        if (!(*sl)[i]->has_placeholder()) return false;
    }
    return true;
  }

  bool Media_Block::is_invisible() const {
    for (size_t i = 0, L = block()->length(); i < L; ++i) {
      Statement_Obj stm = block()->at(i);
      if (!stm->is_invisible()) return false;
    }
    return true;
  }

  Expression_Obj Hashed::at(Expression_Obj k) const
  {
    if (elements_.count(k))
    { return elements_.at(k); }
    else { return {}; }
  }

  bool Binary_Expression::is_left_interpolant(void) const
  {
    return is_interpolant() || (left() && left()->is_left_interpolant());
  }
  bool Binary_Expression::is_right_interpolant(void) const
  {
    return is_interpolant() || (right() && right()->is_right_interpolant());
  }

  const std::string AST_Node::to_string(Sass_Inspect_Options opt) const
  {
    Sass_Output_Options out(opt);
    Emitter emitter(out);
    Inspect i(emitter);
    i.in_declaration = true;
    // ToDo: inspect should be const
    const_cast<AST_Node_Ptr>(this)->perform(&i);
    return i.get_buffer();
  }

  const std::string AST_Node::to_string() const
  {
    return to_string({ NESTED, 5 });
  }

  bool Declaration::is_invisible() const
  {
    if (is_custom_property()) return false;

    return !(value_ && value_->concrete_type() != Expression::NULL_VAL);
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Additional method on Lists to retrieve values directly or from an encompassed Argument.
  //////////////////////////////////////////////////////////////////////////////////////////
  Expression_Obj List::value_at_index(size_t i) {
    Expression_Obj obj = this->at(i);
    if (is_arglist_) {
      if (Argument_Ptr arg = Cast<Argument>(obj)) {
        return arg->value();
      } else {
        return obj;
      }
    } else {
      return obj;
    }
  }

  IMPLEMENT_AST_OPERATORS(Supports_Operator);
  IMPLEMENT_AST_OPERATORS(Supports_Negation);
  IMPLEMENT_AST_OPERATORS(Ruleset);
  IMPLEMENT_AST_OPERATORS(Media_Block);
  IMPLEMENT_AST_OPERATORS(Import);
  IMPLEMENT_AST_OPERATORS(Import_Stub);
  IMPLEMENT_AST_OPERATORS(Directive);
  IMPLEMENT_AST_OPERATORS(At_Root_Block);
  IMPLEMENT_AST_OPERATORS(Supports_Block);
  IMPLEMENT_AST_OPERATORS(While);
  IMPLEMENT_AST_OPERATORS(Each);
  IMPLEMENT_AST_OPERATORS(For);
  IMPLEMENT_AST_OPERATORS(If);
  IMPLEMENT_AST_OPERATORS(Mixin_Call);
  IMPLEMENT_AST_OPERATORS(Extension);
  IMPLEMENT_AST_OPERATORS(Media_Query);
  IMPLEMENT_AST_OPERATORS(Media_Query_Expression);
  IMPLEMENT_AST_OPERATORS(Debug);
  IMPLEMENT_AST_OPERATORS(Error);
  IMPLEMENT_AST_OPERATORS(Warning);
  IMPLEMENT_AST_OPERATORS(Assignment);
  IMPLEMENT_AST_OPERATORS(Return);
  IMPLEMENT_AST_OPERATORS(At_Root_Query);
  IMPLEMENT_AST_OPERATORS(Comment);
  IMPLEMENT_AST_OPERATORS(Supports_Interpolation);
  IMPLEMENT_AST_OPERATORS(Supports_Declaration);
  IMPLEMENT_AST_OPERATORS(Supports_Condition);
  IMPLEMENT_AST_OPERATORS(Parameters);
  IMPLEMENT_AST_OPERATORS(Parameter);
  IMPLEMENT_AST_OPERATORS(Arguments);
  IMPLEMENT_AST_OPERATORS(Argument);
  IMPLEMENT_AST_OPERATORS(Unary_Expression);
  IMPLEMENT_AST_OPERATORS(Block);
  IMPLEMENT_AST_OPERATORS(Content);
  IMPLEMENT_AST_OPERATORS(Trace);
  IMPLEMENT_AST_OPERATORS(Keyframe_Rule);
  IMPLEMENT_AST_OPERATORS(Bubble);
  IMPLEMENT_AST_OPERATORS(Definition);
  IMPLEMENT_AST_OPERATORS(Declaration);
}
