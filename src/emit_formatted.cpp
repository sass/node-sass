#include "emit_formatted.hpp"
#include "ast.hpp"

namespace Sass {
  using namespace std;

  Formatted_Emitter::Formatted_Emitter()
  : buffer(""), indentation(0)
  { }

  Formatted_Emitter::~Formatted_Emitter() { }

  // statements
  void Formatted_Emitter::operator()(Block* block)
  {
    if (!block->is_root()) {
      indent();
      buffer += "{\n";
    }
    ++indentation;
    for (size_t i = 0, L = block->length(); i < L; ++i) {
      indent();
      (*block)[i]->perform(this);
      buffer += '\n';
    }
    --indentation;
    if (!block->is_root()) {
      indent();
      buffer += "}";
    }
  }

  void Formatted_Emitter::operator()(Ruleset* ruleset)
  {
    ruleset->selector()->perform(this);
    ruleset->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Propset* propset)
  {
    propset->property_fragment()->perform(this); buffer += ": ";
    propset->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Media_Block* media_block)
  {
    media_block->media_queries()->perform(this);
    media_block->block()->perform(this);
  }

  void Formatted_Emitter::operator()(At_Rule* at_rule)
  {
    buffer += at_rule->keyword() += ' ';
    if (at_rule->selector()) {
      at_rule->selector()->perform(this);
      buffer += ' ';
    }
    at_rule->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Declaration* dec)
  {
    dec->property()->perform(this);
    buffer += ": ";
    dec->value()->perform(this);
    if (dec->is_important()) buffer += " !important";
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Assignment* assn)
  {
    buffer += assn->variable() += ": ";
    assn->value()->perform(this);
    if (assn->is_guarded()) buffer += " !default";
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Import* import)
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

  void Formatted_Emitter::operator()(Import_Stub* import)
  {
    buffer += "@import ";
    buffer += import->file_name();
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Warning* warning)
  {
    buffer += "@warn ";
    warning->message()->perform(this);
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Comment* comment)
  {
    comment->text()->perform(this);
  }

  void Formatted_Emitter::operator()(If* cond)
  {
    buffer += "@if ";
    cond->predicate()->perform(this);
    buffer += ' ';
    cond->consequent()->perform(this);
    if (cond->alternative()) {
      buffer += "\nelse ";
      cond->alternative()->perform(this);
    }
  }

  void Formatted_Emitter::operator()(For* loop)
  {
    buffer += string("@for ") += loop->variable() += " from ";
    loop->lower_bound()->perform(this);
    buffer += (loop->is_inclusive() ? " through " : " to ");
    loop->upper_bound()->perform(this);
    buffer += ' ';
    loop->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Each* loop)
  {
    buffer += string("@each ") += loop->variable() += " in ";
    loop->list()->perform(this);
    buffer += ' ';
    loop->block()->perform(this);
  }

  void Formatted_Emitter::operator()(While* loop)
  {
    buffer += "@while ";
    loop->predicate()->perform(this);
    buffer += ' ';
    loop->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Return* ret)
  {
    buffer += "@return ";
    ret->value()->perform(this);
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Content* content)
  {
    buffer += "@content;";
  }

  void Formatted_Emitter::operator()(Extend* extend)
  {
    buffer += "@extend ";
    extend->selector()->perform(this);
    buffer += ';';
  }

  void Formatted_Emitter::operator()(Definition* def)
  {
    if (def->type() == Definition::MIXIN) buffer += "@mixin ";
    else                                  buffer += "@function ";
    buffer += def->name();
    def->parameters()->perform(this);
    buffer += ' ';
    def->block()->perform(this);
  }

  void Formatted_Emitter::operator()(Mixin_Call* call)
  {
    buffer += string("@include ") += call->name();
    if (call->block()) {
      buffer += ' ';
      call->block()->perform(this);
    }
    if (!call->block()) buffer += ';';
  }
  // // expressions
  // void Formatted_Emitter::operator()(List*)
  // void Formatted_Emitter::operator()(Binary_Expression*)
  // void Formatted_Emitter::operator()(Unary_Expression*)
  // void Formatted_Emitter::operator()(Function_Call*)
  // void Formatted_Emitter::operator()(Variable*)
  // void Formatted_Emitter::operator()(Textual*)
  // void Formatted_Emitter::operator()(Number*)
  // void Formatted_Emitter::operator()(Percentage*)
  // void Formatted_Emitter::operator()(Dimension*)
  // void Formatted_Emitter::operator()(Color*)
  // void Formatted_Emitter::operator()(Boolean*)
  // void Formatted_Emitter::operator()(String_Schema*)
  // void Formatted_Emitter::operator()(String_Constant*)
  // void Formatted_Emitter::operator()(Media_Query_Expression*)
  // // parameters and arguments
  // void Formatted_Emitter::operator()(Parameter*)
  // void Formatted_Emitter::operator()(Parameters*)
  // void Formatted_Emitter::operator()(Argument*)
  // void Formatted_Emitter::operator()(Arguments*)
  // // selectors
  // void Formatted_Emitter::operator()(Selector_Schema*)
  // void Formatted_Emitter::operator()(Selector_Reference*)
  // void Formatted_Emitter::operator()(Selector_Placeholder*)
  // void Formatted_Emitter::operator()(Type_Selector*)
  // void Formatted_Emitter::operator()(Selector_Qualifier*)
  // void Formatted_Emitter::operator()(Attribute_Selector*)
  // void Formatted_Emitter::operator()(Pseudo_Selector*)
  // void Formatted_Emitter::operator()(Negated_Selector*)
  // void Formatted_Emitter::operator()(Simple_Selector_Sequence*)
  // void Formatted_Emitter::operator()(Selector_Combination*)
  // void Formatted_Emitter::operator()(Selector_Group*)

  void Formatted_Emitter::indent()
  { buffer += string(2*indentation, ' '); }

}