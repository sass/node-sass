#include "ast.hpp"
#include "context.hpp"
#include "to_string.hpp"
#include <set>
#include <algorithm>
#include <iostream>

namespace Sass {
  using namespace std;

  bool Compound_Selector::operator<(const Compound_Selector& rhs) const
  {
    To_String to_string;
    // ugly
    return const_cast<Compound_Selector*>(this)->perform(&to_string) <
           const_cast<Compound_Selector&>(rhs).perform(&to_string);
  }

  bool Compound_Selector::is_superselector_of(Compound_Selector* rhs)
  {
    To_String to_string;

    Simple_Selector* lbase = base();
    Simple_Selector* rbase = rhs->base();

    set<string> lset, rset;
    for (size_t i = 1, L = length(); i < L; ++i)
    { lset.insert((*this)[i]->perform(&to_string)); }
    for (size_t i = 1, L = rhs->length(); i < L; ++i)
    { rset.insert((*rhs)[i]->perform(&to_string)); }

    return (!lbase || (rbase && (lbase->perform(&to_string) == rbase->perform(&to_string)))) &&
           // skip the pseudo-element check for now
           includes(rset.begin(), rset.end(), lset.begin(), lset.end());
  }

  bool Complex_Selector::is_superselector_of(Compound_Selector* rhs)
  {
    if (length() != 1)
    { return false; }
    return base()->is_superselector_of(rhs);
  }

  bool Complex_Selector::is_superselector_of(Complex_Selector* rhs)
  {
    // check for selectors with leading or trailing combinators
    if (!lhs->head() || !rhs->head())
    { return false; }
    Complex_Selector* l_innermost = lhs->innermost();
    if (l_innermost->combinator() != Complex_Selector::ANCESTOR_OF && !l_innermost->tail())
    { return false; }
    Complex_Selector* r_innermost = rhs->innermost();
    if (r_innermost->combinator() != Complex_Selector::ANCESTOR_OF && !r_innermost->tail())
    { return false; }
    // more complex (i.e., longer) selectors are always more specific
    size_t l_len = lhs->length(), r_len = rhs->length();
    if (l_len > r_len)
    { return false; }

    if (l_len == 1)
    { return lhs->head()->is_superselector_of(rhs->base()); }

    bool found = false;
    for (size_t i = 0; 
  }

  size_t Complex_Selector::length()
  {
    // TODO: make this iterative
    if (!tail()) return 1;
    return 1 + tail()->length();
  }

  Compound_Selector* Complex_Selector::base()
  {
    if (!tail()) return head();
    else return tail()->base();
  }

  Complex_Selector* Complex_Selector::context(Context& ctx)
  {
    if (!tail()) return 0;
    if (!head()) return tail()->context(ctx);
    return new (ctx.mem) Complex_Selector(path(), line(), combinator(), head(), tail()->context(ctx));
  }

  Complex_Selector* Complex_Selector::innermost()
  {
    if (!tail()) return this;
    else return tail()->innermost();
  }

  Selector_Placeholder* Selector::find_placeholder()
  {
    return 0;
  }

  Selector_Placeholder* Selector_List::find_placeholder()
  {
    if (has_placeholder()) {
      for (size_t i = 0, L = length(); i < L; ++i) {
        if ((*this)[i]->has_placeholder()) return (*this)[i]->find_placeholder();
      }
    }
    return 0;
  }

  Selector_Placeholder* Complex_Selector::find_placeholder()
  {
    if (has_placeholder()) {
      if (head() && head()->has_placeholder()) return head()->find_placeholder();
      else if (tail() && tail()->has_placeholder()) return tail()->find_placeholder();
    }
    return 0;
  }

  Selector_Placeholder* Compound_Selector::find_placeholder()
  {
    if (has_placeholder()) {
      for (size_t i = 0, L = length(); i < L; ++i) {
        if ((*this)[i]->has_placeholder()) return (*this)[i]->find_placeholder();
      }
      // return this;
    }
    return 0;
  }

  Selector_Placeholder* Selector_Placeholder::find_placeholder()
  {
    return this;
  }

}