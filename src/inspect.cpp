#include "inspect.hpp"
#include "ast.hpp"
#include "to_string.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  Inspect::Inspect()
  : to_string(new To_String()), buffer(""), indentation(0)
  { }

  Inspect::~Inspect()
  { delete to_string; }

  // statements
  void Inspect::operator()(Block* block)
  {
    if (!block->is_root()) {
      buffer += " {\n";
      ++indentation;
    }
    for (size_t i = 0, L = block->length(); i < L; ++i) {
      indent();
      (*block)[i]->perform(this);
      // extra newline at the end of top-level statements
      if (block->is_root()) buffer += '\n';
      buffer += '\n';
    }
    if (!block->is_root()) {
      --indentation;
      indent();
      buffer += "}";
    }
    // remove extra newline that gets added after the last top-level block
    if (block->is_root()) {
      size_t l = buffer.length();
      if (l > 2 && buffer[l-1] == '\n' && buffer[l-2] == '\n')
        buffer.erase(l-1);
    }
  }

  void Inspect::operator()(Ruleset* ruleset)
  {
    ruleset->selector()->perform(this);
    ruleset->block()->perform(this);
  }

  void Inspect::operator()(Propset* propset)
  {
    propset->property_fragment()->perform(this);
    buffer += ": {\n";
    ++indentation;
    for (size_t i = 0, S = propset->declarations().size(); i < S; ++i) {
      indent();
      propset->declarations()[i]->perform(this);
      buffer += '\n';
    }
    for (size_t i = 0, S = propset->propsets().size(); i < S; ++i) {
      indent();
      propset->propsets()[i]->perform(this);
      buffer += '\n';
    }
    --indentation;
    buffer += "}";
  }

  void Inspect::operator()(Media_Block* media_block)
  {
    buffer += "@media ";
    media_block->media_queries()->perform(this);
    media_block->block()->perform(this);
  }

  void Inspect::operator()(At_Rule* at_rule)
  {
    buffer += at_rule->keyword();
    if (at_rule->selector()) {
      buffer += ' ';
      at_rule->selector()->perform(this);
    }
    if (at_rule->block()) {
      at_rule->block()->perform(this);
    }
    else {
      buffer += ';';
    }
  }

  void Inspect::operator()(Declaration* dec)
  {
    dec->property()->perform(this);
    buffer += ": ";
    dec->value()->perform(this);
    if (dec->is_important()) buffer += " !important";
    buffer += ';';
  }

  void Inspect::operator()(Assignment* assn)
  {
    buffer += assn->variable();
    buffer += ": ";
    assn->value()->perform(this);
    if (assn->is_guarded()) buffer += " !default";
    buffer += ';';
  }

  void Inspect::operator()(Import* import)
  {
    if (!import->urls().empty()) {
      buffer += "@import ";
      import->urls().front()->perform(this);
      for (size_t i = 1, S = import->urls().size(); i < S; ++i) {
        buffer += ", ";
        import->urls()[i]->perform(this);
      }
      buffer += ';';
    }
  }

  void Inspect::operator()(Import_Stub* import)
  {
    buffer += "@import ";
    buffer += import->file_name();
    buffer += ';';
  }

  void Inspect::operator()(Warning* warning)
  {
    buffer += "@warn ";
    warning->message()->perform(this);
    buffer += ';';
  }

  void Inspect::operator()(Comment* comment)
  {
    comment->text()->perform(this);
  }

  void Inspect::operator()(If* cond)
  {
    buffer += "@if ";
    cond->predicate()->perform(this);
    cond->consequent()->perform(this);
    if (cond->alternative()) {
      buffer += '\n';
      indent();
      buffer += "else";
      cond->alternative()->perform(this);
    }
  }

  void Inspect::operator()(For* loop)
  {
    buffer += string("@for ");
    buffer += loop->variable();
    buffer += " from ";
    loop->lower_bound()->perform(this);
    buffer += (loop->is_inclusive() ? " through " : " to ");
    loop->upper_bound()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(Each* loop)
  {
    buffer += string("@each ");
    buffer += loop->variable();
    buffer += " in ";
    loop->list()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(While* loop)
  {
    buffer += "@while ";
    loop->predicate()->perform(this);
    loop->block()->perform(this);
  }

  void Inspect::operator()(Return* ret)
  {
    buffer += "@return ";
    ret->value()->perform(this);
    buffer += ';';
  }

  void Inspect::operator()(Extend* extend)
  {
    buffer += "@extend ";
    extend->selector()->perform(this);
    buffer += ';';
  }

  void Inspect::operator()(Definition* def)
  {
    if (def->type() == Definition::MIXIN) buffer += "@mixin ";
    else                                  buffer += "@function ";
    buffer += def->name();
    def->parameters()->perform(this);
    def->block()->perform(this);
  }

  void Inspect::operator()(Mixin_Call* call)
  {
    buffer += string("@include ") += call->name();
    if (call->arguments()) {
      call->arguments()->perform(this);
    }
    if (call->block()) {
      buffer += ' ';
      call->block()->perform(this);
    }
    if (!call->block()) buffer += ';';
  }

  void Inspect::operator()(Content* content)
  {
    buffer += "@content;";
  }

  // expressions
  // void Inspect::operator()(Expression* expr)
  // {
  //   buffer += expr->perform(to_string);
  // }

  // void Inspect::operator()(List*)

  void Inspect::operator()(Binary_Expression* expr)
  {
    expr->left()->perform(this);
    switch (expr->type()) {
      case Binary_Expression::AND: buffer += " and "; break;
      case Binary_Expression::OR:  buffer += " or ";  break;
      case Binary_Expression::EQ:  buffer += " == ";  break;
      case Binary_Expression::NEQ: buffer += " != ";  break;
      case Binary_Expression::GT:  buffer += " > ";   break;
      case Binary_Expression::GTE: buffer += " >= ";  break;
      case Binary_Expression::LT:  buffer += " < ";   break;
      case Binary_Expression::LTE: buffer += " <= ";  break;
      case Binary_Expression::ADD: buffer += " + ";   break;
      case Binary_Expression::SUB: buffer += " - ";   break;
      case Binary_Expression::MUL: buffer += " * ";   break;
      case Binary_Expression::DIV: buffer += " / ";   break;
      case Binary_Expression::MOD: buffer += " % ";   break;
    }
    expr->right()->perform(this);
  }

  void Inspect::operator()(Unary_Expression* expr)
  {
    if (expr->type() == Unary_Expression::PLUS) buffer += '+';
    else                                        buffer += '-';
    expr->operand()->perform(this);
  }

  // void Inspect::operator()(Function_Call*)

  void Inspect::operator()(Variable* var)
  {
    buffer += var->name();
  }

  // void Inspect::operator()(Textual*)
  // void Inspect::operator()(Number*)
  // void Inspect::operator()(Percentage*)
  // void Inspect::operator()(Dimension*)
  // void Inspect::operator()(Color*)
  // void Inspect::operator()(Boolean*)

  // void Inspect::operator()(String_Schema* ss)
  // {
  //   buffer += "#{";
  //   for (size_t i = 0, L = ss->length(); i < L; ++i) (*ss)[i]->perform(this);
  //   buffer += '}';
  // }

  // void Inspect::operator()(String_Constant*)
  // void Inspect::operator()(Media_Query*)
  // void Inspect::operator()(Media_Query_Expression*)

  // parameters and arguments
  void Inspect::operator()(Parameter* p)
  {
    buffer += p->name();
    if (p->default_value()) {
      buffer += ": ";
      p->default_value()->perform(this);
    }
    else if (p->is_rest_parameter()) {
      buffer += "...";
    }
  }

  void Inspect::operator()(Parameters* p)
  {
    buffer += '(';
    if (!p->empty()) {
      (*p)[0]->perform(this);
      for (size_t i = 1, L = p->length(); i < L; ++i) {
        buffer += ", ";
        (*p)[i]->perform(this);
      }
    }
    buffer += ')';
  }

  // void Inspect::operator()(Argument* a)
  // {
  //   buffer += a->perform(to_string);
  // }

  // void Inspect::operator()(Arguments* a)
  // {
  //   buffer += a->perform(to_string);
  // }

  // selectors
  // void Inspect::operator()(Selector_Schema* s)
  // {
  //   s->contents()->perform(this);
  // }

  // void Inspect::operator()(Selector_Reference*)
  // void Inspect::operator()(Selector_Placeholder*)
  // void Inspect::operator()(Type_Selector*)
  // void Inspect::operator()(Selector_Qualifier*)
  // void Inspect::operator()(Attribute_Selector*)
  // void Inspect::operator()(Pseudo_Selector*)
  // void Inspect::operator()(Negated_Selector*)
  // void Inspect::operator()(Simple_Selector_Sequence*)
  // void Inspect::operator()(Selector_Combination*)
  // void Inspect::operator()(Selector_Group*)

  inline void Inspect::fallback_impl(AST_Node* n)
  { buffer += n->perform(to_string); }

  void Inspect::indent()
  { buffer += string(2*indentation, ' '); }

}