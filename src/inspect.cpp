#include "inspect.hpp"
#include "ast.hpp"
// #include "to_string.hpp"
#include <iostream>

namespace Sass {
  using namespace std;

  Inspector::Inspector()
  : to_string(new To_String()), buffer(""), indentation(0)
  { }

  Inspector::~Inspector()
  { delete to_string; }

  // statements
  void Inspector::operator()(Block* block)
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

  void Inspector::operator()(Ruleset* ruleset)
  {
    ruleset->selector()->perform(this);
    ruleset->block()->perform(this);
  }

  void Inspector::operator()(Propset* propset)
  {
    propset->property_fragment()->perform(this);
    buffer += ":";
    propset->block()->perform(this);
  }

  void Inspector::operator()(Media_Block* media_block)
  {
    buffer += "@media ";
    media_block->media_queries()->perform(this);
    media_block->block()->perform(this);
  }

  void Inspector::operator()(At_Rule* at_rule)
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

  void Inspector::operator()(Declaration* dec)
  {
    dec->property()->perform(this);
    buffer += ": ";
    dec->value()->perform(this);
    if (dec->is_important()) buffer += " !important";
    buffer += ';';
  }

  void Inspector::operator()(Assignment* assn)
  {
    buffer += assn->variable();
    buffer += ": ";
    assn->value()->perform(this);
    if (assn->is_guarded()) buffer += " !default";
    buffer += ';';
  }

  void Inspector::operator()(Import* import)
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

  void Inspector::operator()(Import_Stub* import)
  {
    buffer += "@import ";
    buffer += import->file_name();
    buffer += ';';
  }

  void Inspector::operator()(Warning* warning)
  {
    buffer += "@warn ";
    warning->message()->perform(this);
    buffer += ';';
  }

  void Inspector::operator()(Comment* comment)
  {
    comment->text()->perform(this);
  }

  void Inspector::operator()(If* cond)
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

  void Inspector::operator()(For* loop)
  {
    buffer += string("@for ");
    buffer += loop->variable();
    buffer += " from ";
    loop->lower_bound()->perform(this);
    buffer += (loop->is_inclusive() ? " through " : " to ");
    loop->upper_bound()->perform(this);
    loop->block()->perform(this);
  }

  void Inspector::operator()(Each* loop)
  {
    buffer += string("@each ");
    buffer += loop->variable();
    buffer += " in ";
    loop->list()->perform(this);
    loop->block()->perform(this);
  }

  void Inspector::operator()(While* loop)
  {
    buffer += "@while ";
    loop->predicate()->perform(this);
    loop->block()->perform(this);
  }

  void Inspector::operator()(Return* ret)
  {
    buffer += "@return ";
    ret->value()->perform(this);
    buffer += ';';
  }

  void Inspector::operator()(Content* content)
  {
    buffer += "@content;";
  }

  void Inspector::operator()(Extend* extend)
  {
    buffer += "@extend ";
    extend->selector()->perform(this);
    buffer += ';';
  }

  void Inspector::operator()(Definition* def)
  {
    if (def->type() == Definition::MIXIN) buffer += "@mixin ";
    else                                  buffer += "@function ";
    buffer += def->name();
    def->parameters()->perform(this);
    def->block()->perform(this);
  }

  void Inspector::operator()(Mixin_Call* call)
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

  // expressions
  // void Inspector::operator()(Expression* expr)
  // {
  //   buffer += expr->perform(to_string);
  // }

  // void Inspector::operator()(List*)

  void Inspector::operator()(Binary_Expression* expr)
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

  void Inspector::operator()(Unary_Expression* expr)
  {
    if (expr->type() == Unary_Expression::PLUS) buffer += '+';
    else                                        buffer += '-';
    expr->operand()->perform(this);
  }

  // void Inspector::operator()(Function_Call*)

  void Inspector::operator()(Variable* var)
  {
    buffer += var->name();
  }

  // void Inspector::operator()(Textual*)
  // void Inspector::operator()(Number*)
  // void Inspector::operator()(Percentage*)
  // void Inspector::operator()(Dimension*)
  // void Inspector::operator()(Color*)
  // void Inspector::operator()(Boolean*)

  // void Inspector::operator()(String_Schema* ss)
  // {
  //   buffer += "#{";
  //   for (size_t i = 0, L = ss->length(); i < L; ++i) (*ss)[i]->perform(this);
  //   buffer += '}';
  // }

  // void Inspector::operator()(String_Constant*)
  // void Inspector::operator()(Media_Query*)
  // void Inspector::operator()(Media_Query_Expression*)

  // parameters and arguments
  void Inspector::operator()(Parameter* p)
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

  void Inspector::operator()(Parameters* p)
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

  // void Inspector::operator()(Argument* a)
  // {
  //   buffer += a->perform(to_string);
  // }

  // void Inspector::operator()(Arguments* a)
  // {
  //   buffer += a->perform(to_string);
  // }

  // selectors
  // void Inspector::operator()(Selector_Schema* s)
  // {
  //   s->contents()->perform(this);
  // }

  // void Inspector::operator()(Selector_Reference*)
  // void Inspector::operator()(Selector_Placeholder*)
  // void Inspector::operator()(Type_Selector*)
  // void Inspector::operator()(Selector_Qualifier*)
  // void Inspector::operator()(Attribute_Selector*)
  // void Inspector::operator()(Pseudo_Selector*)
  // void Inspector::operator()(Negated_Selector*)
  // void Inspector::operator()(Simple_Selector_Sequence*)
  // void Inspector::operator()(Selector_Combination*)
  // void Inspector::operator()(Selector_Group*)

  // void Inspector::fallback(AST_Node* n)
  // { buffer += n->perform(to_string); }

  void Inspector::indent()
  { buffer += string(2*indentation, ' '); }

}