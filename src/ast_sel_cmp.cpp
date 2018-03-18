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

#include "ast_selectors.hpp"

namespace Sass {

  bool Compound_Selector::operator< (const Compound_Selector& rhs) const
  {
    size_t L = std::min(length(), rhs.length());
    for (size_t i = 0; i < L; ++i)
    {
      Simple_Selector_Obj l = (*this)[i];
      Simple_Selector_Obj r = rhs[i];
      if (!l && !r) return false;
      else if (!r) return false;
      else if (!l) return true;
      else if (*l != *r)
      { return *l < *r; }
    }
    // just compare the length now
    return length() < rhs.length();
  }

  bool Compound_Selector::operator== (const Compound_Selector& rhs) const
  {
    if (&rhs == this) return true;
    if (rhs.length() != length()) return false;
    std::unordered_set<const Simple_Selector *, HashPtr, ComparePtrs> lhs_set;
    lhs_set.reserve(length());
    for (const Simple_Selector_Obj &element : elements()) {
      lhs_set.insert(element.ptr());
    }
    // there is no break?!
    for (const Simple_Selector_Obj &element : rhs.elements()) {
      if (lhs_set.find(element.ptr()) == lhs_set.end()) return false;
    }
    return true;
  }

  bool Complex_Selector::operator< (const Complex_Selector& rhs) const
  {
    // const iterators for tails
    Complex_Selector_Ptr_Const l = this;
    Complex_Selector_Ptr_Const r = &rhs;
    Compound_Selector_Ptr l_h = NULL;
    Compound_Selector_Ptr r_h = NULL;
    if (l) l_h = l->head();
    if (r) r_h = r->head();
    // process all tails
    while (true)
    {
      #ifdef DEBUG
      // skip empty ancestor first
      if (l && l->is_empty_ancestor())
      {
        l_h = NULL;
        l = l->tail();
        if(l) l_h = l->head();
        continue;
      }
      // skip empty ancestor first
      if (r && r->is_empty_ancestor())
      {
        r_h = NULL;
        r = r->tail();
        if (r) r_h = r->head();
        continue;
      }
      #endif
      // check for valid selectors
      if (!l) return !!r;
      if (!r) return false;
      // both are null
      else if (!l_h && !r_h)
      {
        // check combinator after heads
        if (l->combinator() != r->combinator())
        { return l->combinator() < r->combinator(); }
        // advance to next tails
        l = l->tail();
        r = r->tail();
        // fetch the next headers
        l_h = NULL; r_h = NULL;
        if (l) l_h = l->head();
        if (r) r_h = r->head();
      }
      // one side is null
      else if (!r_h) return true;
      else if (!l_h) return false;
      // heads ok and equal
      else if (*l_h == *r_h)
      {
        // check combinator after heads
        if (l->combinator() != r->combinator())
        { return l->combinator() < r->combinator(); }
        // advance to next tails
        l = l->tail();
        r = r->tail();
        // fetch the next headers
        l_h = NULL; r_h = NULL;
        if (l) l_h = l->head();
        if (r) r_h = r->head();
      }
      // heads are not equal
      else return *l_h < *r_h;
    }
  }

  bool Complex_Selector::operator== (const Complex_Selector& rhs) const
  {
    // const iterators for tails
    Complex_Selector_Ptr_Const l = this;
    Complex_Selector_Ptr_Const r = &rhs;
    Compound_Selector_Ptr l_h = NULL;
    Compound_Selector_Ptr r_h = NULL;
    if (l) l_h = l->head();
    if (r) r_h = r->head();
    // process all tails
    while (true)
    {
      #ifdef DEBUG
      // skip empty ancestor first
      if (l && l->is_empty_ancestor())
      {
        l_h = NULL;
        l = l->tail();
        if (l) l_h = l->head();
        continue;
      }
      // skip empty ancestor first
      if (r && r->is_empty_ancestor())
      {
        r_h = NULL;
        r = r->tail();
        if (r) r_h = r->head();
        continue;
      }
      #endif
      // check the pointers
      if (!r) return !l;
      if (!l) return !r;
      // both are null
      if (!l_h && !r_h)
      {
        // check combinator after heads
        if (l->combinator() != r->combinator())
        { return l->combinator() < r->combinator(); }
        // advance to next tails
        l = l->tail();
        r = r->tail();
        // fetch the next heads
        l_h = NULL; r_h = NULL;
        if (l) l_h = l->head();
        if (r) r_h = r->head();
      }
      // equals if other head is empty
      else if ((!l_h && !r_h) ||
               (!l_h && r_h->empty()) ||
               (!r_h && l_h->empty()) ||
               (l_h && r_h && *l_h == *r_h))
      {
        // check combinator after heads
        if (l->combinator() != r->combinator())
        { return l->combinator() == r->combinator(); }
        // advance to next tails
        l = l->tail();
        r = r->tail();
        // fetch the next heads
        l_h = NULL; r_h = NULL;
        if (l) l_h = l->head();
        if (r) r_h = r->head();
      }
      // abort
      else break;
    }
    // unreachable
    return false;
  }

  bool Complex_Selector::operator== (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this == *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }


  bool Complex_Selector::operator< (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this < *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Compound_Selector::operator== (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this == *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Compound_Selector::operator< (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this < *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Selector_Schema::operator== (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this == *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Selector_Schema::operator< (const Selector& rhs) const
  {
    if (const Selector_List* sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (const Simple_Selector* sp = Cast<Simple_Selector>(&rhs)) return *this < *sp;
    if (const Complex_Selector* cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (const Compound_Selector* ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Simple_Selector::operator== (const Selector& rhs) const
  {
    if (Simple_Selector_Ptr_Const sp = Cast<Simple_Selector>(&rhs)) return *this == *sp;
    return false;
  }

  bool Simple_Selector::operator< (const Selector& rhs) const
  {
    if (Simple_Selector_Ptr_Const sp = Cast<Simple_Selector>(&rhs)) return *this < *sp;
    return false;
  }

  bool Simple_Selector::operator== (const Simple_Selector& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (const Pseudo_Selector* lhs = Cast<Pseudo_Selector>(this)) {return *lhs == rhs; }
    else if (const Wrapped_Selector* lhs = Cast<Wrapped_Selector>(this)) {return *lhs == rhs; }
    else if (const Element_Selector* lhs = Cast<Element_Selector>(this)) {return *lhs == rhs; }
    else if (const Attribute_Selector* lhs = Cast<Attribute_Selector>(this)) {return *lhs == rhs; }
    else if (name_ == rhs.name_)
    { return is_ns_eq(rhs); }
    else return false;
  }

  bool Simple_Selector::operator< (const Simple_Selector& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (const Pseudo_Selector* lhs = Cast<Pseudo_Selector>(this)) {return *lhs < rhs; }
    else if (const Wrapped_Selector* lhs = Cast<Wrapped_Selector>(this)) {return *lhs < rhs; }
    else if (const Element_Selector* lhs = Cast<Element_Selector>(this)) {return *lhs < rhs; }
    else if (const Attribute_Selector* lhs = Cast<Attribute_Selector>(this)) {return *lhs < rhs; }
    if (is_ns_eq(rhs))
    { return name_ < rhs.name_; }
    return ns_ < rhs.ns_;
  }

  bool Selector_List::operator== (const Selector& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (Selector_List_Ptr_Const sl = Cast<Selector_List>(&rhs)) { return *this == *sl; }
    else if (Complex_Selector_Ptr_Const cpx = Cast<Complex_Selector>(&rhs)) { return *this == *cpx; }
    else if (Compound_Selector_Ptr_Const cpd = Cast<Compound_Selector>(&rhs)) { return *this == *cpd; }
    // no compare method
    return this == &rhs;
  }

  // Selector lists can be compared to comma lists
  bool Selector_List::operator== (const Expression& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (List_Ptr_Const ls = Cast<List>(&rhs)) { return *ls == *this; }
    if (Selector_Ptr_Const ls = Cast<Selector>(&rhs)) { return *this == *ls; }
    // compare invalid (maybe we should error?)
    return false;
  }

  bool Selector_List::operator== (const Selector_List& rhs) const
  {
    // for array access
    size_t i = 0, n = 0;
    size_t iL = length();
    size_t nL = rhs.length();
    // create temporary vectors and sort them
    std::vector<Complex_Selector_Obj> l_lst = this->elements();
    std::vector<Complex_Selector_Obj> r_lst = rhs.elements();
    std::sort(l_lst.begin(), l_lst.end(), OrderNodes());
    std::sort(r_lst.begin(), r_lst.end(), OrderNodes());
    // process loop
    while (true)
    {
      // first check for valid index
      if (i == iL) return iL == nL;
      else if (n == nL) return iL == nL;
      // the access the vector items
      Complex_Selector_Obj l = l_lst[i];
      Complex_Selector_Obj r = r_lst[n];
      // skip nulls
      if (!l) ++i;
      else if (!r) ++n;
      // do the check
      else if (*l != *r)
      { return false; }
      // advance
      ++i; ++n;
    }
    // there is no break?!
  }

  bool Selector_List::operator< (const Selector& rhs) const
  {
    if (Selector_List_Ptr_Const sp = Cast<Selector_List>(&rhs)) return *this < *sp;
    return false;
  }

  bool Selector_List::operator< (const Selector_List& rhs) const
  {
    size_t l = rhs.length();
    if (length() < l) l = length();
    for (size_t i = 0; i < l; i ++) {
      if (*at(i) < *rhs.at(i)) return true;
    }
    return false;
  }

  bool Attribute_Selector::operator< (const Attribute_Selector& rhs) const
  {
    if (is_ns_eq(rhs)) {
      if (name() == rhs.name()) {
        if (matcher() == rhs.matcher()) {
          bool no_lhs_val = value().isNull();
          bool no_rhs_val = rhs.value().isNull();
          if (no_lhs_val && no_rhs_val) return false; // equal
          else if (no_lhs_val) return true; // lhs is null
          else if (no_rhs_val) return false; // rhs is null
          return *value() < *rhs.value(); // both are given
        } else { return matcher() < rhs.matcher(); }
      } else { return name() < rhs.name(); }
    } else { return ns() < rhs.ns(); }
  }

  bool Attribute_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (Attribute_Selector_Ptr_Const w = Cast<Attribute_Selector>(&rhs))
    {
      return *this < *w;
    }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Attribute_Selector::operator== (const Attribute_Selector& rhs) const
  {
    // get optional value state
    bool no_lhs_val = value().isNull();
    bool no_rhs_val = rhs.value().isNull();
    // both are null, therefore equal
    if (no_lhs_val && no_rhs_val) {
      return (name() == rhs.name())
        && (matcher() == rhs.matcher())
        && (is_ns_eq(rhs));
    }
    // both are defined, evaluate
    if (no_lhs_val == no_rhs_val) {
      return (name() == rhs.name())
        && (matcher() == rhs.matcher())
        && (is_ns_eq(rhs))
        && (*value() == *rhs.value());
    }
    // not equal
    return false;

  }

  bool Attribute_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (Attribute_Selector_Ptr_Const w = Cast<Attribute_Selector>(&rhs))
    {
      return is_ns_eq(rhs) &&
             name() == rhs.name() &&
             *this == *w;
    }
    return false;
  }

  bool Element_Selector::operator< (const Element_Selector& rhs) const
  {
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Element_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (Element_Selector_Ptr_Const w = Cast<Element_Selector>(&rhs))
    {
      return *this < *w;
    }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Element_Selector::operator== (const Element_Selector& rhs) const
  {
    return is_ns_eq(rhs) &&
           name() == rhs.name();
  }

  bool Element_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (Element_Selector_Ptr_Const w = Cast<Element_Selector>(&rhs))
    {
      return is_ns_eq(rhs) &&
             name() == rhs.name() &&
             *this == *w;
    }
    return false;
  }

  bool Pseudo_Selector::operator== (const Pseudo_Selector& rhs) const
  {
    if (is_ns_eq(rhs) && name() == rhs.name())
    {
      String_Obj lhs_ex = expression();
      String_Obj rhs_ex = rhs.expression();
      if (rhs_ex && lhs_ex) return *lhs_ex == *rhs_ex;
      else return lhs_ex.ptr() == rhs_ex.ptr();
    }
    else return false;
  }

  bool Pseudo_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (Pseudo_Selector_Ptr_Const w = Cast<Pseudo_Selector>(&rhs))
    {
      return *this == *w;
    }
    return is_ns_eq(rhs) &&
           name() == rhs.name();
  }

  bool Pseudo_Selector::operator< (const Pseudo_Selector& rhs) const
  {
    if (is_ns_eq(rhs) && name() == rhs.name())
    {
      String_Obj lhs_ex = expression();
      String_Obj rhs_ex = rhs.expression();
      if (rhs_ex && lhs_ex) return *lhs_ex < *rhs_ex;
      else return lhs_ex.ptr() < rhs_ex.ptr();
    }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Pseudo_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (Pseudo_Selector_Ptr_Const w = Cast<Pseudo_Selector>(&rhs))
    {
      return *this < *w;
    }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Wrapped_Selector::operator== (const Wrapped_Selector& rhs) const
  {
    if (is_ns_eq(rhs) && name() == rhs.name())
    { return *(selector()) == *(rhs.selector()); }
    else return false;
  }

  bool Wrapped_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (Wrapped_Selector_Ptr_Const w = Cast<Wrapped_Selector>(&rhs))
    {
      return *this == *w;
    }
    return is_ns_eq(rhs) &&
           name() == rhs.name();
  }

  bool Wrapped_Selector::operator< (const Wrapped_Selector& rhs) const
  {
    if (is_ns_eq(rhs) && name() == rhs.name())
    { return *(selector()) < *(rhs.selector()); }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Wrapped_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (Wrapped_Selector_Ptr_Const w = Cast<Wrapped_Selector>(&rhs))
    {
      return *this < *w;
    }
    if (is_ns_eq(rhs))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

}