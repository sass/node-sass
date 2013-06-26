#include "output_compressed.hpp"
#include "inspect.hpp"
#include "ast.hpp"

namespace Sass {
  using namespace std;

  Output_Compressed::Output_Compressed() : buffer("") { }
  Output_Compressed::~Output_Compressed() { }

  inline void Output_Compressed::fallback_impl(AST_Node* n)
  {
    Inspect i;
    n->perform(&i);
    buffer += i.get_buffer();
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

    if (b->has_non_hoistable()) {
      s->perform(this);
      buffer += "{";
      for (size_t i = 0, L = b->length(); i < L; ++i) {
        Statement* stm = (*b)[i];
        if (!stm->is_hoistable()) {
          stm->perform(this);
        }
      }
      buffer += "}";
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

    buffer += "@media ";
    q->perform(this);
    buffer += "{";

    Selector* e = m->enclosing_selector();
    bool hoisted = false;
    if (e && b->has_non_hoistable()) {
      hoisted = true;
      e->perform(this);
      buffer += "{";
    }

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (!stm->is_hoistable()) {
        stm->perform(this);
      }
    }

    if (hoisted) {
      buffer += "}";
    }

    for (size_t i = 0, L = b->length(); i < L; ++i) {
      Statement* stm = (*b)[i];
      if (stm->is_hoistable()) {
        stm->perform(this);
      }
    }

    buffer += "}";
  }

  void Output_Compressed::operator()(At_Rule* a)
  {
    string    kwd   = a->keyword();
    Selector* s     = a->selector();
    Block*    b     = a->block();

    buffer += kwd;
    if (s) {
      buffer += ' ';
      s->perform(this);
    }

    if (!b) {
      buffer += ';';
      return;
    }

    buffer += "{";
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

    buffer += "}";
  }

  void Output_Compressed::operator()(Declaration* d)
  {
    d->property()->perform(this);
    buffer += ":";
    d->value()->perform(this);
    if (d->is_important()) buffer += "!important";
    buffer += ';';
  }

}