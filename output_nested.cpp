#include <iostream>
#include <sstream>
#include <typeinfo>

#include "ast.hpp"
#include "util.hpp"
#include "context.hpp"
#include "inspect.hpp"
#include "to_string.hpp"
#include "output_nested.hpp"

namespace Sass {
  using namespace std;

  Output_Nested::Output_Nested(bool source_comments, Context* ctx)
  : Output(ctx),
    indentation(0),
    source_comments(source_comments),
    in_directive(false),
    in_keyframes(false)
  { }
  Output_Nested::~Output_Nested() { }

  inline void Output_Nested::fallback_impl(AST_Node* n)
  {
    Inspect i(ctx);
    n->perform(&i);
    append_to_buffer(i.get_buffer());
  }

  void Output_Nested::operator()(Import* imp)
  {
    // Inspect insp(ctx);
    // imp->perform(&insp);
    // insp.get_buffer();
    top_imports.push_back(imp);
  }

  void Output_Nested::operator()(Block* b)
  {
    if (!b->is_root()) return;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      size_t old_len = buffer.length();
      (*b)[i]->perform(this);
      if (i < L-1 && old_len < buffer.length()) append_to_buffer(ctx->linefeed);
    }
  }

  void Output_Nested::operator()(Comment* c)
  {
    To_String to_string;
    string txt = c->text()->perform(&to_string);
    if (buffer.size() + top_imports.size() == 0) {
      top_comments.push_back(c);
    } else {
      Inspect i(ctx);
      c->perform(&i);
      append_to_buffer(i.get_buffer());
    }
  }

  void Output_Nested::operator()(Ruleset* r)
  {
    Selector* s     = r->selector();
    Block*    b     = r->block();
    bool      decls = false;

    // disabled to avoid clang warning [-Wunused-function]
    // Selector_List* sl = static_cast<Selector_List*>(s);

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
      decls = true;
      indentation += r->tabs();
      indent();
      if (source_comments) {
        stringstream ss;
        ss << "/* line " << r->pstate().line+1 << ", " << r->pstate().path << " */" << endl;
        append_to_buffer(ss.str());
        indent();
      }
      s->perform(this);
      append_to_buffer(" {" + ctx->linefeed);
      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        bool bPrintExpression = true;
        // Check print conditions
        if (typeid(*stm) == typeid(Declaration)) {
          Declaration* dec = static_cast<Declaration*>(stm);
          if (dec->value()->concrete_type() == Expression::STRING) {
            String_Constant* valConst = static_cast<String_Constant*>(dec->value());
            string val(valConst->value());
            if (val.empty()) {
              bPrintExpression = false;
            }
          }
          else if (dec->value()->concrete_type() == Expression::LIST) {
            List* list = static_cast<List*>(dec->value());
            bool all_invisible = true;
            for (size_t list_i = 0, list_L = list->length(); list_i < list_L; ++list_i) {
              Expression* item = (*list)[list_i];
              if (!item->is_invisible()) all_invisible = false;
            }
            if (all_invisible) bPrintExpression = false;
          }
        }
        // Print if OK
        if (!stm->is_hoistable() && bPrintExpression) {
          if (!stm->block()) indent();
          stm->perform(this);
          if (i < L-1) append_to_buffer(ctx->linefeed);
        }
      }
      --indentation;
      indentation -= r->tabs();

      while (buffer.substr(buffer.length()-ctx->linefeed.length()) == ctx->linefeed) {
        buffer.erase(buffer.length()-1);
        if (ctx) ctx->source_map.remove_line();
      }

      append_to_buffer(" }");
      if (r->group_end()) append_to_buffer(ctx->linefeed);
      // Match Sass 3.4.9 behaviour
      if (in_directive && !in_keyframes) append_to_buffer(ctx->linefeed);
    }

    if (b->has_hoistable()) {
      if (decls) ++indentation;
      // indent();
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      if (decls) --indentation;
    }
  }

  void Output_Nested::operator()(Feature_Block* f)
  {
    if (f->is_invisible()) return;

    Feature_Query* q    = f->feature_queries();
    Block* b            = f->block();

    // Filter out feature blocks that aren't printable (process its children though)
    if (!Util::isPrintable(f)) {
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (dynamic_cast<Has_Block*>(stm)) {
          stm->perform(this);
        }
      }
      return;
    }

    indentation += f->tabs();
    indent();
    append_to_buffer("@supports", f);
    append_to_buffer(" ");
    q->perform(this);
    append_to_buffer(" ");
    append_to_buffer("{");
    append_to_buffer(ctx->linefeed);

    bool old_in_directive = in_directive;
    in_directive = true;

    Selector* e = f->selector();
    if (e && b->has_non_hoistable()) {
      // JMA - hoisted, output the non-hoistable in a nested block, followed by the hoistable
      ++indentation;
      indent();
      e->perform(this);
      append_to_buffer(" {" + ctx->linefeed);

      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          if (!stm->block()) indent();
          stm->perform(this);
          if (i < L-1) append_to_buffer(ctx->linefeed);
        }
      }
      --indentation;

      in_directive = old_in_directive;

      // buffer.erase(buffer.length()-1);
      // if (ctx) ctx->source_map.remove_line();
      append_to_buffer(" }" + ctx->linefeed);
      --indentation;

      ++indentation;
      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      --indentation;
      --indentation;
    }
    else {
      // JMA - not hoisted, just output in order
      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          if (!stm->block()) indent();
        }
        stm->perform(this);
        if (!stm->is_hoistable()) append_to_buffer(ctx->linefeed);
      }
      --indentation;
    }

    while (buffer.substr(buffer.length()-ctx->linefeed.length()) == ctx->linefeed) {
      buffer.erase(buffer.length()-1);
      if (ctx) ctx->source_map.remove_line();
    }

    in_directive = old_in_directive;

    append_to_buffer(" }");
    if (f->group_end() || in_directive) append_to_buffer(ctx->linefeed);

    indentation -= f->tabs();
  }

  void Output_Nested::operator()(Media_Block* m)
  {
    if (m->is_invisible()) return;

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

    indentation += m->tabs();
    indent();
    append_to_buffer("@media", m);
    append_to_buffer(" ");
    q->perform(this);
    append_to_buffer(" ");
    append_to_buffer("{");
    append_to_buffer(ctx->linefeed);

    bool old_in_directive = in_directive;
    in_directive = true;

    Selector* e = m->selector();
    if (e && b->has_non_hoistable()) {
      // JMA - hoisted, output the non-hoistable in a nested block, followed by the hoistable
      ++indentation;
      indent();
      e->perform(this);
      append_to_buffer(" {" + ctx->linefeed);

      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          if (!stm->block()) indent();
          stm->perform(this);
          if (i < L-1) append_to_buffer(ctx->linefeed);
        }
      }
      --indentation;

      in_directive = old_in_directive;

      // buffer.erase(buffer.length()-1);
      // if (ctx) ctx->source_map.remove_line();
      append_to_buffer(" }" + ctx->linefeed);
      --indentation;

      ++indentation;
      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      --indentation;
      --indentation;
    }
    else {
      // JMA - not hoisted, just output in order
      ++indentation;
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          if (!stm->block()) indent();
        }
        stm->perform(this);
        if (!stm->is_hoistable()) append_to_buffer(ctx->linefeed);
      }
      --indentation;
    }

    while (buffer.substr(buffer.length()-ctx->linefeed.length()) == ctx->linefeed) {
      buffer.erase(buffer.length()-1);
      if (ctx) ctx->source_map.remove_line();
    }

    in_directive = old_in_directive;

    append_to_buffer(" }");
    if (m->group_end() || in_directive) append_to_buffer(ctx->linefeed);

    indentation -= m->tabs();
  }

  void Output_Nested::operator()(At_Rule* a)
  {
    string      kwd   = a->keyword();
    Selector*   s     = a->selector();
    Expression* v     = a->value();
    Block*      b     = a->block();
    bool        decls = false;

    // indent();
    append_to_buffer(kwd);
    if (s) {
      append_to_buffer(" ");
      s->perform(this);
    }
    else if (v) {
      append_to_buffer(" ");
      v->perform(this);
    }

    if (!b) {
      append_to_buffer(";");
      return;
    }

    append_to_buffer(" {" + ctx->linefeed);

    bool old_in_directive = in_directive;
    in_directive = true;
    in_keyframes = kwd.compare("@keyframes") == 0;

    ++indentation;
    decls = true;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (!stm->is_hoistable()) {
        if (!stm->block()) indent();
        stm->perform(this);
        append_to_buffer(ctx->linefeed);
      }
    }
    --indentation;

    if (decls) ++indentation;
    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (stm->is_hoistable()) {
        stm->perform(this);
        // append_to_buffer(ctx->linefeed);
      }
    }
    if (decls) --indentation;

    while (buffer.substr(buffer.length()-ctx->linefeed.length()) == ctx->linefeed) {
      buffer.erase(buffer.length()-1);
      if (ctx) ctx->source_map.remove_line();
    }

    in_directive = old_in_directive;
    in_keyframes = false;

    append_to_buffer(" }");

    // Match Sass 3.4.9 behaviour
    if (kwd.compare("@font-face") != 0 && kwd.compare("@keyframes") != 0) append_to_buffer(ctx->linefeed);
  }

  void Output_Nested::indent()
  {
    string indent = "";
    for (size_t i = 0; i < indentation; i++)
      indent += ctx->indent;
    append_to_buffer(indent);
  }

  // compile output implementation
  template class Output<Output_Nested>;

}
