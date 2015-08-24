#include <iostream>
#include <typeinfo>
#include <string>

#include "listize.hpp"
#include "to_string.hpp"
#include "context.hpp"
#include "backtrace.hpp"
#include "error_handling.hpp"

namespace Sass {

  Listize::Listize(Context& ctx)
  : ctx(ctx)
  {  }

  Expression* Listize::operator()(Selector_List* sel)
  {
    List* l = SASS_MEMORY_NEW(ctx.mem, List, sel->pstate(), sel->length(), SASS_COMMA);
    for (size_t i = 0, L = sel->length(); i < L; ++i) {
      if (!(*sel)[i]) continue;
      *l << (*sel)[i]->perform(this);
    }
    return l;
  }

  Expression* Listize::operator()(Compound_Selector* sel)
  {
    To_String to_string;
    std::string str;
    for (size_t i = 0, L = sel->length(); i < L; ++i) {
      Expression* e = (*sel)[i]->perform(this);
      if (e) str += e->perform(&to_string);
    }
    return SASS_MEMORY_NEW(ctx.mem, String_Quoted, sel->pstate(), str);
  }

  Expression* Listize::operator()(Complex_Selector* sel)
  {
    List* l = SASS_MEMORY_NEW(ctx.mem, List, sel->pstate(), 2);

    Compound_Selector* head = sel->head();
    if (head && !head->is_empty_reference())
    {
      Expression* hh = head->perform(this);
      if (hh) *l << hh;
    }

    To_String to_string;
    std::string reference = ! sel->reference() ? ""
      : sel->reference()->perform(&to_string);
    switch(sel->combinator())
    {
      case Complex_Selector::PARENT_OF:
        *l << SASS_MEMORY_NEW(ctx.mem, String_Quoted, sel->pstate(), ">");
      break;
      case Complex_Selector::ADJACENT_TO:
        *l << SASS_MEMORY_NEW(ctx.mem, String_Quoted, sel->pstate(), "+");
      break;
      case Complex_Selector::REFERENCE:
        *l << SASS_MEMORY_NEW(ctx.mem, String_Quoted, sel->pstate(), "/" + reference + "/");
      break;
      case Complex_Selector::PRECEDES:
        *l << SASS_MEMORY_NEW(ctx.mem, String_Quoted, sel->pstate(), "~");
      break;
      case Complex_Selector::ANCESTOR_OF:
      break;
    }

    Complex_Selector* tail = sel->tail();
    if (tail)
    {
      Expression* tt = tail->perform(this);
      if (tt && tt->concrete_type() == Expression::LIST)
      { *l += static_cast<List*>(tt); }
      else if (tt) *l << static_cast<List*>(tt);
    }
    if (l->length() == 0) return 0;
    return l;
  }

  Expression* Listize::fallback_impl(AST_Node* n)
  {
    return dynamic_cast<Expression*>(n);
  }

}
