#include "output_compressed.hpp"
#include "inspect.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "to_string.hpp"
#include "util.hpp"

namespace Sass {
  using namespace std;

  Output_Compressed::Output_Compressed(Context* ctx) : buffer(""), rendered_imports(""), ctx(ctx) { }
  Output_Compressed::~Output_Compressed() { }

  inline void Output_Compressed::fallback_impl(AST_Node* n)
  {
    Inspect i(ctx);
    n->perform(&i);
    buffer += i.get_buffer();
  }

  void Output_Compressed::operator()(Import* imp)
  {
    Inspect insp(ctx);
    imp->perform(&insp);
    rendered_imports += insp.get_buffer();
  }

  void Output_Compressed::operator()(Block* b)
  {
    if (!b->is_root()) return;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      (*b)[i]->perform(this);
    }
  }

  void Output_Compressed::operator()(Ruleset* r)
  {
    Selector* s     = r->selector();
    Block*    b     = r->block();

    // Filter out rulesets that aren't printable (process its children though)
    if (!Util::isPrintable(r)) {
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (dynamic_cast<Has_Block*>(stm)) {
          stm->perform(this);
        }
      }
      return;
    }

    if (b->has_non_hoistable()) {
      s->perform(this);
      append_singleline_part_to_buffer("{");
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      size_t l = buffer.length();
      buffer.erase(l-1);
      append_singleline_part_to_buffer("}");
    }

    if (b->has_hoistable()) {
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (stm->is_hoistable()) {
          stm->perform(this);
        }
      }
    }
  }

  void Output_Compressed::operator()(Media_Block* m)
  {
    List*  q     = m->media_queries();
    Block* b     = m->block();

    // Filter out media blocks that aren't printable (process its children though)
    if (!Util::isPrintable(m)) {
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (dynamic_cast<Has_Block*>(stm)) {
          stm->perform(this);
        }
      }
      return;
    }
    
    ctx->source_map.add_mapping(m);
    append_singleline_part_to_buffer("@media ");
    q->perform(this);
    append_singleline_part_to_buffer("{");

    Selector* e = m->selector();
    if (e && b->has_non_hoistable()) {
      // JMA - hoisted, output the non-hoistable in a nested block, followed by the hoistable
      e->perform(this);
      append_singleline_part_to_buffer("{");

      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      
      append_singleline_part_to_buffer("}");
      
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (stm->is_hoistable()) {
          stm->perform(this);
        }
      }
    }
    else {
      // JMA - not hoisted, just output in order
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          stm->perform(this);
        }
      }
    }

    append_singleline_part_to_buffer("}");
  }

  void Output_Compressed::operator()(At_Rule* a)
  {
    string      kwd   = a->keyword();
    Selector*   s     = a->selector();
    Expression* v     = a->value();
    Block*      b     = a->block();

    append_singleline_part_to_buffer(kwd);
    if (s) {
      append_singleline_part_to_buffer(" ");
      s->perform(this);
    }
    else if (v) {
      append_singleline_part_to_buffer(" ");
      v->perform(this);
    }

    if (!b) {
      append_singleline_part_to_buffer(";");
      return;
    }

    append_singleline_part_to_buffer("{");
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (!stm->is_hoistable()) {
        stm->perform(this);
      }
    }

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (stm->is_hoistable()) {
        stm->perform(this);
      }
    }

    append_singleline_part_to_buffer("}");
  }

  void Output_Compressed::operator()(Declaration* d)
  {
    bool bPrintExpression = true;
    // Check print conditions
    if (d->value()->concrete_type() == Expression::NULL_VAL) {
      bPrintExpression = false;
    }
    if (d->value()->concrete_type() == Expression::STRING) {
      String_Constant* valConst = static_cast<String_Constant*>(d->value());
      string val(valConst->value());
      if (val.empty()) {
        bPrintExpression = false;
      }
    }
    // Print if OK
    if(bPrintExpression) {
      if (ctx) ctx->source_map.add_mapping(d->property());
      d->property()->perform(this);
      append_singleline_part_to_buffer(":");
      if (ctx) ctx->source_map.add_mapping(d->value());
      d->value()->perform(this);
      if (d->is_important()) append_singleline_part_to_buffer("!important");
      append_singleline_part_to_buffer(";");
    }
  }

  void Output_Compressed::operator()(Comment* c)
  {
    To_String to_string;
    string txt = c->text()->perform(&to_string);
    if(txt[2] != '!') {
      return;
    }
    else {
      Inspect i(ctx);
      c->perform(&i);
      buffer += i.get_buffer();
    }
  }

  void Output_Compressed::operator()(List* list)
  {
    string sep(list->separator() == List::SPACE ? " " : ",");
    if (list->empty()) return;
    Expression* first = (*list)[0];
    bool first_invisible = first->is_invisible();
    if (!first_invisible) first->perform(this);
    for (size_t i = 1, L = list->length(); i < L; ++i) {
      Expression* next = (*list)[i];
      bool next_invisible = next->is_invisible();
      if (i == 1 && !first_invisible && !next_invisible) append_singleline_part_to_buffer(sep);
      else if (!next_invisible)                          append_singleline_part_to_buffer(sep);
      next->perform(this);
    }
  }

  void Output_Compressed::operator()(Media_Query_Expression* mqe)
  {
    if (mqe->is_interpolated()) {
      mqe->feature()->perform(this);
    }
    else {
      append_singleline_part_to_buffer("(");
      mqe->feature()->perform(this);
      if (mqe->value()) {
        append_singleline_part_to_buffer(":");
        mqe->value()->perform(this);
      }
      append_singleline_part_to_buffer(")");
    }
  }

  void Output_Compressed::operator()(Null* n)
  {
    // noop
  }

  void Output_Compressed::operator()(Argument* a)
  {
    if (!a->name().empty()) {
      append_singleline_part_to_buffer(a->name());
      append_singleline_part_to_buffer(":");
    }
    a->value()->perform(this);
    if (a->is_rest_argument()) {
      append_singleline_part_to_buffer("...");
    }
  }

  void Output_Compressed::operator()(Arguments* a)
  {
    append_singleline_part_to_buffer("(");
    if (!a->empty()) {
      (*a)[0]->perform(this);
      for (size_t i = 1, L = a->length(); i < L; ++i) {
        append_singleline_part_to_buffer(",");
        (*a)[i]->perform(this);
      }
    }
    append_singleline_part_to_buffer(")");
  }

  void Output_Compressed::operator()(Complex_Selector* c)
  {
    Compound_Selector*           head = c->head();
    Complex_Selector*            tail = c->tail();
    Complex_Selector::Combinator comb = c->combinator();
    if (head && head->is_empty_reference() && tail)
    {
      tail->perform(this);
      return;
    } 
    if (head && !head->is_empty_reference()) head->perform(this);
    switch (comb) {
      case Complex_Selector::ANCESTOR_OF:
        if (tail) append_singleline_part_to_buffer(" ");
        break;
      case Complex_Selector::PARENT_OF:
        append_singleline_part_to_buffer(">");
        break;
      case Complex_Selector::PRECEDES:
        // Apparently need to preserve spaces around this combinator?
        if (head && !head->is_empty_reference()) append_singleline_part_to_buffer(" ");
        append_singleline_part_to_buffer("~");
        if (tail) append_singleline_part_to_buffer(" ");
        break;
      case Complex_Selector::ADJACENT_TO:
        append_singleline_part_to_buffer("+");
        break;
    }
    if (tail) tail->perform(this);
  }

  void Output_Compressed::operator()(Selector_List* g)
  {
    if (g->empty()) return;
    (*g)[0]->perform(this);
    for (size_t i = 1, L = g->length(); i < L; ++i) {
      append_singleline_part_to_buffer(",");
      (*g)[i]->perform(this);
    }
  }

  void Output_Compressed::append_singleline_part_to_buffer(const string& text)
  {
    buffer += text;
    if (ctx) ctx->source_map.update_column(text);
  }

}
