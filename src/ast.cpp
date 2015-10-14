#include "ast.hpp"
#include "context.hpp"
#include "node.hpp"
#include "extend.hpp"
#include "to_string.hpp"
#include "color_maps.hpp"
#include <set>
#include <iomanip>
#include <algorithm>
#include <iostream>

namespace Sass {

  static Null sass_null(Sass::Null(ParserState("null")));

  bool Supports_Operator::needs_parens(Supports_Condition* cond) const {
    return dynamic_cast<Supports_Negation*>(cond) ||
          (dynamic_cast<Supports_Operator*>(cond) &&
           dynamic_cast<Supports_Operator*>(cond)->operand() != operand());
  }

  bool Supports_Negation::needs_parens(Supports_Condition* cond) const {
    return dynamic_cast<Supports_Negation*>(cond) ||
          dynamic_cast<Supports_Operator*>(cond);
  }

  void AST_Node::update_pstate(const ParserState& pstate)
  {
    pstate_.offset += pstate - pstate_ + pstate.offset;
  }

  inline bool is_ns_eq(const std::string& l, const std::string& r)
  {
    if (l.empty() && r.empty()) return true;
    else if (l.empty() && r == "*") return true;
    else if (r.empty() && l == "*") return true;
    else return l == r;
  }



  bool Compound_Selector::operator< (const Compound_Selector& rhs) const
  {
    size_t L = std::min(length(), rhs.length());
    for (size_t i = 0; i < L; ++i)
    {
      Simple_Selector* l = (*this)[i];
      Simple_Selector* r = rhs[i];
      if (!l && !r) return false;
      else if (!r) return false;
      else if (!l) return true;
      else if (*l != *r)
      { return *l < *r; }
    }
    // just compare the length now
    return length() < rhs.length();
  }

  bool Compound_Selector::has_parent_ref()
  {
    return has_parent_reference();
  }

  bool Complex_Selector::has_parent_ref()
  {
    return (head() && head()->has_parent_ref()) ||
           (tail() && tail()->has_parent_ref());
  }

  bool Complex_Selector::operator< (const Complex_Selector& rhs) const
  {
    // const iterators for tails
    const Complex_Selector* l = this;
    const Complex_Selector* r = &rhs;
    Compound_Selector* l_h = l ? l->head() : 0;
    Compound_Selector* r_h = r ? r->head() : 0;
    // process all tails
    while (true)
    {
      // skip empty ancestor first
      if (l && l->is_empty_ancestor())
      {
        l = l->tail();
        l_h = l ? l->head() : 0;
        continue;
      }
      // skip empty ancestor first
      if (r && r->is_empty_ancestor())
      {
        r = r->tail();
        r_h = r ? r->head() : 0;
        continue;
      }
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
        l_h = l ? l->head() : 0;
        r_h = r ? r->head() : 0;
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
        l_h = l ? l->head() : 0;
        r_h = r ? r->head() : 0;
      }
      // heads are not equal
      else return *l_h < *r_h;
    }
    return true;
  }

  bool Complex_Selector::operator== (const Complex_Selector& rhs) const
  {
    // const iterators for tails
    const Complex_Selector* l = this;
    const Complex_Selector* r = &rhs;
    Compound_Selector* l_h = l ? l->head() : 0;
    Compound_Selector* r_h = r ? r->head() : 0;
    // process all tails
    while (true)
    {
      // skip empty ancestor first
      if (l && l->is_empty_ancestor())
      {
        l = l->tail();
        l_h = l ? l->head() : 0;
        continue;
      }
      // skip empty ancestor first
      if (r && r->is_empty_ancestor())
      {
        r = r->tail();
        r_h = r ? r->head() : 0;
        continue;
      }
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
        l_h = l ? l->head() : 0;
        r_h = r ? r->head() : 0;
      }
      // fail if only one is null
      else if (!r_h) return !l_h;
      else if (!l_h) return !r_h;
      // heads ok and equal
      else if (*l_h == *r_h)
      {
        // check combinator after heads
        if (l->combinator() != r->combinator())
        { return l->combinator() == r->combinator(); }
        // advance to next tails
        l = l->tail();
        r = r->tail();
        // fetch the next heads
        l_h = l ? l->head() : 0;
        r_h = r ? r->head() : 0;
      }
      // abort
      else break;
    }
    // unreachable
    return false;
  }

  Compound_Selector* Compound_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    Compound_Selector* unified = rhs;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if (!unified) break;
      unified = (*this)[i]->unify_with(unified, ctx);
    }
    return unified;
  }

  bool Simple_Selector::operator== (const Simple_Selector& rhs) const
  {
    const Attribute_Selector* ll = dynamic_cast<const Attribute_Selector*>(this);
    const Attribute_Selector* rr = dynamic_cast<const Attribute_Selector*>(&rhs);
    if (ll && rr) return *ll == *rr;

    if (is_ns_eq(ns(), rhs.ns()))
    { return name() == rhs.name(); }
    return ns() == rhs.ns();
  }

  bool Simple_Selector::operator< (const Simple_Selector& rhs) const
  {
    const Attribute_Selector* ll = dynamic_cast<const Attribute_Selector*>(this);
    const Attribute_Selector* rr = dynamic_cast<const Attribute_Selector*>(&rhs);
    if (ll && rr) return *ll < *rr;

    if (is_ns_eq(ns(), rhs.ns()))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Selector_List::operator== (const Selector& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (const Selector_List* ls = dynamic_cast<const Selector_List*>(&rhs)) { return *this == *ls; }
    else if (const Complex_Selector* ls = dynamic_cast<const Complex_Selector*>(&rhs)) { return *this == *ls; }
    else if (const Compound_Selector* ls = dynamic_cast<const Compound_Selector*>(&rhs)) { return *this == *ls; }
    // no compare method
    return this == &rhs;
  }

  // Selector lists can be compared to comma lists
  bool Selector_List::operator==(const Expression& rhs) const
  {
    // solve the double dispatch problem by using RTTI information via dynamic cast
    if (const List* ls = dynamic_cast<const List*>(&rhs)) { return *this == *ls; }
    if (const Selector* ls = dynamic_cast<const Selector*>(&rhs)) { return *this == *ls; }
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
    std::vector<Complex_Selector*> l_lst = this->elements();
    std::vector<Complex_Selector*> r_lst = rhs.elements();
    std::sort(l_lst.begin(), l_lst.end(), cmp_complex_selector());
    std::sort(r_lst.begin(), r_lst.end(), cmp_complex_selector());
    // process loop
    while (true)
    {
      // first check for valid index
      if (i == iL) return iL == nL;
      else if (n == nL) return iL == nL;
      // the access the vector items
      Complex_Selector* l = l_lst[i];
      Complex_Selector* r = r_lst[n];
      // skip nulls
      if (!l) ++i;
      else if (!r) ++n;
      // do the check
      else if (*l != *r)
      { return false; }
      // advance
      ++i; ++n;
    }
    // no mismatch
    return true;
  }

  Compound_Selector* Simple_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    To_String to_string(&ctx);
    for (size_t i = 0, L = rhs->length(); i < L; ++i)
    { if (perform(&to_string) == (*rhs)[i]->perform(&to_string)) return rhs; }

    // check for pseudo elements because they are always last
    size_t i, L;
    bool found = false;
    if (typeid(*this) == typeid(Pseudo_Selector) || typeid(*this) == typeid(Wrapped_Selector))
    {
      for (i = 0, L = rhs->length(); i < L; ++i)
      {
        if ((typeid(*(*rhs)[i]) == typeid(Pseudo_Selector) || typeid(*(*rhs)[i]) == typeid(Wrapped_Selector)) && (*rhs)[L-1]->is_pseudo_element())
        { found = true; break; }
      }
    }
    else
    {
      for (i = 0, L = rhs->length(); i < L; ++i)
      {
        if (typeid(*(*rhs)[i]) == typeid(Pseudo_Selector) || typeid(*(*rhs)[i]) == typeid(Wrapped_Selector))
        { found = true; break; }
      }
    }
    if (!found)
    {
      Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, *rhs);
      (*cpy) << this;
      return cpy;
    }
    Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, rhs->pstate());
    for (size_t j = 0; j < i; ++j)
    { (*cpy) << (*rhs)[j]; }
    (*cpy) << this;
    for (size_t j = i; j < L; ++j)
    { (*cpy) << (*rhs)[j]; }
    return cpy;
  }

  Simple_Selector* Type_Selector::unify_with(Simple_Selector* rhs, Context& ctx)
  {
    // check if ns can be extended
    // true for no ns or universal
    if (has_universal_ns())
    {
      // but dont extend with universal
      // true for valid ns and universal
      if (!rhs->is_universal_ns())
      {
        // creaty the copy inside (avoid unnecessary copies)
        Type_Selector* ts = SASS_MEMORY_NEW(ctx.mem, Type_Selector, *this);
        // overwrite the name if star is given as name
        if (ts->name() == "*") { ts->name(rhs->name()); }
        // now overwrite the namespace name and flag
        ts->ns(rhs->ns()); ts->has_ns(rhs->has_ns());
        // return copy
        return ts;
      }
    }
    // namespace may changed, check the name now
    // overwrite star (but not with another star)
    if (name() == "*" && rhs->name() != "*")
    {
      // creaty the copy inside (avoid unnecessary copies)
      Type_Selector* ts = SASS_MEMORY_NEW(ctx.mem, Type_Selector, *this);
      // simply set the new name
      ts->name(rhs->name());
      // return copy
      return ts;
    }
    // return original
    return this;
  }

  Compound_Selector* Type_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    // TODO: handle namespaces

    // if the rhs is empty, just return a copy of this
    if (rhs->length() == 0) {
      Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, rhs->pstate());
      (*cpy) << this;
      return cpy;
    }

    Simple_Selector* rhs_0 = (*rhs)[0];
    // otherwise, this is a tag name
    if (name() == "*")
    {
      if (typeid(*rhs_0) == typeid(Type_Selector))
      {
        // if rhs is universal, just return this tagname + rhs's qualifiers
        Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, *rhs);
        Type_Selector* ts = static_cast<Type_Selector*>(rhs_0);
        (*cpy)[0] = this->unify_with(ts, ctx);
        return cpy;
      }
      else if (dynamic_cast<Selector_Qualifier*>(rhs_0)) {
        // qualifier is `.class`, so we can prefix with `ns|*.class`
        Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, rhs->pstate());
        if (has_ns() && !rhs_0->has_ns()) {
          if (ns() != "*") (*cpy) << this;
        }
        for (size_t i = 0, L = rhs->length(); i < L; ++i)
        { (*cpy) << (*rhs)[i]; }
        return cpy;
      }


      return rhs;
    }

    if (typeid(*rhs_0) == typeid(Type_Selector))
    {
      // if rhs is universal, just return this tagname + rhs's qualifiers
      if (rhs_0->name() != "*" && rhs_0->ns() != "*" && rhs_0->name() != name()) return 0;
      // otherwise create new compound and unify first simple selector
      Compound_Selector* copy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, *rhs);
      (*copy)[0] = this->unify_with(rhs_0, ctx);
      return copy;

    }
    // else it's a tag name and a bunch of qualifiers -- just append them
    Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, rhs->pstate());
    if (name() != "*") (*cpy) << this;
    (*cpy) += rhs;
    return cpy;
  }

  Compound_Selector* Selector_Qualifier::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    if (name()[0] == '#')
    {
      for (size_t i = 0, L = rhs->length(); i < L; ++i)
      {
        Simple_Selector* rhs_i = (*rhs)[i];
        if (typeid(*rhs_i) == typeid(Selector_Qualifier) &&
            static_cast<Selector_Qualifier*>(rhs_i)->name()[0] == '#' &&
            static_cast<Selector_Qualifier*>(rhs_i)->name() != name())
          return 0;
      }
    }
    rhs->has_line_break(has_line_break());
    return Simple_Selector::unify_with(rhs, ctx);
  }

  Compound_Selector* Pseudo_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    if (is_pseudo_element())
    {
      for (size_t i = 0, L = rhs->length(); i < L; ++i)
      {
        Simple_Selector* rhs_i = (*rhs)[i];
        if (typeid(*rhs_i) == typeid(Pseudo_Selector) &&
            static_cast<Pseudo_Selector*>(rhs_i)->is_pseudo_element() &&
            static_cast<Pseudo_Selector*>(rhs_i)->name() != name())
        { return 0; }
      }
    }
    return Simple_Selector::unify_with(rhs, ctx);
  }

  bool Attribute_Selector::operator< (const Attribute_Selector& rhs) const
  {
    if (is_ns_eq(ns(), rhs.ns())) {
      if (name() == rhs.name()) {
        if (matcher() == rhs.matcher()) {
          return value() < rhs.value();
        } else { return matcher() < rhs.matcher(); }
      } else { return name() < rhs.name(); }
    }
    else return false;
  }

  bool Attribute_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (const Attribute_Selector* w = dynamic_cast<const Attribute_Selector*>(&rhs))
    {
      return *this < *w;
    }
    if (is_ns_eq(ns(), rhs.ns()))
    { return name() < rhs.name(); }
    return ns() < rhs.ns();
  }

  bool Attribute_Selector::operator== (const Attribute_Selector& rhs) const
  {
    if (is_ns_eq(ns(), rhs.ns()) && name() == rhs.name())
    { return matcher() == rhs.matcher() && value() == rhs.value(); }
    else return false;
  }

  bool Attribute_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (const Attribute_Selector* w = dynamic_cast<const Attribute_Selector*>(&rhs))
    {
      return *this == *w;
    }
    if (is_ns_eq(ns(), rhs.ns()))
    { return name() == rhs.name(); }
    return ns() == rhs.ns();
  }

  bool Wrapped_Selector::operator== (const Wrapped_Selector& rhs) const
  {
    if (is_ns_eq(ns(), rhs.ns()) && name() == rhs.name())
    { return *(selector()) == *(rhs.selector()); }
    else return false;
  }

  bool Wrapped_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (const Wrapped_Selector* w = dynamic_cast<const Wrapped_Selector*>(&rhs))
    {
      return *this == *w;
    }
    if (is_ns_eq(ns(), rhs.ns()))
    { return name() == rhs.name(); }
    return ns() == rhs.ns();
  }

  bool Wrapped_Selector::is_superselector_of(Wrapped_Selector* sub)
  {
    if (this->name() != sub->name()) return false;
    if (this->name() == ":current") return false;
    if (Selector_List* rhs_list = dynamic_cast<Selector_List*>(sub->selector())) {
      if (Selector_List* lhs_list = dynamic_cast<Selector_List*>(selector())) {
        return lhs_list->is_superselector_of(rhs_list);
      }
      error("is_superselector expected a Selector_List", sub->pstate());
    } else {
      error("is_superselector expected a Selector_List", sub->pstate());
    }
    return false;
  }

  bool Compound_Selector::is_superselector_of(Selector_List* rhs, std::string wrapped)
  {
    for (Complex_Selector* item : rhs->elements()) {
      if (is_superselector_of(item, wrapped)) return true;
    }
    return false;
  }

  bool Compound_Selector::is_superselector_of(Complex_Selector* rhs, std::string wrapped)
  {
    if (rhs->head()) return is_superselector_of(rhs->head(), wrapped);
    return false;
  }

  bool Compound_Selector::is_superselector_of(Compound_Selector* rhs, std::string wrapping)
  {
    To_String to_string;

    Compound_Selector* lhs = this;
    Simple_Selector* lbase = lhs->base();
    Simple_Selector* rbase = rhs->base();

    // Check if pseudo-elements are the same between the selectors

    std::set<std::string> lpsuedoset, rpsuedoset;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if ((*this)[i]->is_pseudo_element()) {
        std::string pseudo((*this)[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        lpsuedoset.insert(pseudo);
      }
    }
    for (size_t i = 0, L = rhs->length(); i < L; ++i)
    {
      if ((*rhs)[i]->is_pseudo_element()) {
        std::string pseudo((*rhs)[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        rpsuedoset.insert(pseudo);
      }
    }
    if (lpsuedoset != rpsuedoset) {
      return false;
    }

    std::set<std::string> lset, rset;

    if (lbase && rbase)
    {
      if (lbase->perform(&to_string) == rbase->perform(&to_string)) {
        for (size_t i = 1, L = length(); i < L; ++i)
        { lset.insert((*this)[i]->perform(&to_string)); }
        for (size_t i = 1, L = rhs->length(); i < L; ++i)
        { rset.insert((*rhs)[i]->perform(&to_string)); }
        return includes(rset.begin(), rset.end(), lset.begin(), lset.end());
      }
      return false;
    }

    for (size_t i = 0, iL = length(); i < iL; ++i)
    {
      Selector* lhs = (*this)[i];
      // very special case for wrapped matches selector
      if (Wrapped_Selector* wrapped = dynamic_cast<Wrapped_Selector*>(lhs)) {
        if (wrapped->name() == ":not") {
          if (Selector_List* not_list = dynamic_cast<Selector_List*>(wrapped->selector())) {
            if (not_list->is_superselector_of(rhs, wrapped->name())) return false;
          } else {
            throw std::runtime_error("wrapped not selector is not a list");
          }
        }
        if (wrapped->name() == ":matches" || wrapped->name() == ":-moz-any") {
          lhs = wrapped->selector();
          if (Selector_List* list = dynamic_cast<Selector_List*>(wrapped->selector())) {
            if (Compound_Selector* comp = dynamic_cast<Compound_Selector*>(rhs)) {
              if (!wrapping.empty() && wrapping != wrapped->name()) return false;
              if (wrapping.empty() || wrapping != wrapped->name()) {;
                if (list->is_superselector_of(comp, wrapped->name())) return true;
              }
            }
          }
        }
        Simple_Selector* rhs_sel = rhs->elements().size() > i ? (*rhs)[i] : 0;
        if (Wrapped_Selector* wrapped_r = dynamic_cast<Wrapped_Selector*>(rhs_sel)) {
          if (wrapped->name() == wrapped_r->name()) {
          if (wrapped->is_superselector_of(wrapped_r)) {
             continue;
             rset.insert(lhs->perform(&to_string));

          }}
        }
      }
      // match from here on as strings
      lset.insert(lhs->perform(&to_string));
    }

    for (size_t n = 0, nL = rhs->length(); n < nL; ++n)
    {
      auto r = (*rhs)[n];
      if (Wrapped_Selector* wrapped = dynamic_cast<Wrapped_Selector*>(r)) {
        if (wrapped->name() == ":not") {
          if (Selector_List* ls = dynamic_cast<Selector_List*>(wrapped->selector())) {
            ls->remove_parent_selectors();
            if (is_superselector_of(ls, wrapped->name())) return false;
          }
        }
        if (wrapped->name() == ":matches" || wrapped->name() == ":-moz-any") {
          if (!wrapping.empty()) {
            if (wrapping != wrapped->name()) return false;
          }
          if (Selector_List* ls = dynamic_cast<Selector_List*>(wrapped->selector())) {
            ls->remove_parent_selectors();
            return (is_superselector_of(ls, wrapped->name()));
          }
        }
      }
      rset.insert(r->perform(&to_string));
    }

    //for (auto l : lset) { cerr << "l: " << l << endl; }
    //for (auto r : rset) { cerr << "r: " << r << endl; }

    if (lset.empty()) return true;
    // return true if rset contains all the elements of lset
    return includes(rset.begin(), rset.end(), lset.begin(), lset.end());

  }

  // create complex selector (ancestor of) from compound selector
  Complex_Selector* Compound_Selector::to_complex(Memory_Manager& mem)
  {
    // create an intermediate complex selector
    return SASS_MEMORY_NEW(mem, Complex_Selector,
                           pstate(),
                           Complex_Selector::ANCESTOR_OF,
                           this,
                           0);
  }

  Selector_List* Complex_Selector::unify_with(Complex_Selector* other, Context& ctx)
  {

    // get last tails (on the right side)
    Complex_Selector* l_last = this->last();
    Complex_Selector* r_last = other->last();

    // check valid pointers (assertion)
    SASS_ASSERT(l_last, "lhs is null");
    SASS_ASSERT(r_last, "rhs is null");

    // Not sure about this check, but closest way I could check
    // was to see if this is a ruby 'SimpleSequence' equivalent.
    // It seems to do the job correctly as some specs react to this
    if (l_last->combinator() != Combinator::ANCESTOR_OF) return 0;
    if (r_last->combinator() != Combinator::ANCESTOR_OF ) return 0;

    // get the headers for the last tails
    Compound_Selector* l_last_head = l_last->head();
    Compound_Selector* r_last_head = r_last->head();

    // check valid head pointers (assertion)
    SASS_ASSERT(l_last_head, "lhs head is null");
    SASS_ASSERT(r_last_head, "rhs head is null");

    // get the unification of the last compound selectors
    Compound_Selector* unified = r_last_head->unify_with(l_last_head, ctx);

    // abort if we could not unify heads
    if (unified == 0) return 0;

    // check for universal (star: `*`) selector
    bool is_universal = l_last_head->is_universal() ||
                        r_last_head->is_universal();

    if (is_universal)
    {
      // move the head
      l_last->head(0);
      r_last->head(unified);
    }

    // create nodes from both selectors
    Node lhsNode = complexSelectorToNode(this, ctx);
    Node rhsNode = complexSelectorToNode(other, ctx);

    // overwrite universal base
    if (!is_universal)
    {
      // create some temporaries to convert to node
      Complex_Selector* fake = unified->to_complex(ctx.mem);
      Node unified_node = complexSelectorToNode(fake, ctx);
      // add to permutate the list?
      rhsNode.plus(unified_node);
    }

    // do some magic we inherit from node and extend
    Node node = Extend::subweave(lhsNode, rhsNode, ctx);
    Selector_List* result = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
    NodeDequePtr col = node.collection(); // move from collection to list
    for (NodeDeque::iterator it = col->begin(), end = col->end(); it != end; it++)
    { (*result) << nodeToComplexSelector(Node::naiveTrim(*it, ctx), ctx); }

    // only return if list has some entries
    return result->length() ? result : 0;

  }

  bool Compound_Selector::operator== (const Compound_Selector& rhs) const
  {
    // for array access
    size_t i = 0, n = 0;
    size_t iL = length();
    size_t nL = rhs.length();
    // create temporary vectors and sort them
    std::vector<Simple_Selector*> l_lst = this->elements();
    std::vector<Simple_Selector*> r_lst = rhs.elements();
    std::sort(l_lst.begin(), l_lst.end(), cmp_simple_selector());
    std::sort(r_lst.begin(), r_lst.end(), cmp_simple_selector());
    // process loop
    while (true)
    {
      // first check for valid index
      if (i == iL) return iL == nL;
      else if (n == nL) return iL == nL;
      // the access the vector items
      Simple_Selector* l = l_lst[i];
      Simple_Selector* r = r_lst[n];
      // skip nulls
      if (!l) ++i;
      if (!r) ++n;
      // do the check now
      else if (*l != *r)
      { return false; }
      // advance now
      ++i; ++n;
    }
    // no mismatch
    return true;
  }

  bool Complex_Selector_Pointer_Compare::operator() (const Complex_Selector* const pLeft, const Complex_Selector* const pRight) const {
    return *pLeft < *pRight;
  }

  bool Complex_Selector::is_superselector_of(Compound_Selector* rhs, std::string wrapping)
  {
    return last()->head() && last()->head()->is_superselector_of(rhs, wrapping);
  }

  bool Complex_Selector::is_superselector_of(Complex_Selector* rhs, std::string wrapping)
  {
    Complex_Selector* lhs = this;
    To_String to_string;
    // check for selectors with leading or trailing combinators
    if (!lhs->head() || !rhs->head())
    { return false; }
    const Complex_Selector* l_innermost = lhs->innermost();
    if (l_innermost->combinator() != Complex_Selector::ANCESTOR_OF)
    { return false; }
    const Complex_Selector* r_innermost = rhs->innermost();
    if (r_innermost->combinator() != Complex_Selector::ANCESTOR_OF)
    { return false; }
    // more complex (i.e., longer) selectors are always more specific
    size_t l_len = lhs->length(), r_len = rhs->length();
    if (l_len > r_len)
    { return false; }

    if (l_len == 1)
    { return lhs->head()->is_superselector_of(rhs->last()->head(), wrapping); }

    // we have to look one tail deeper, since we cary the
    // combinator around for it (which is important here)
    if (rhs->tail() && lhs->tail() && combinator() != Complex_Selector::ANCESTOR_OF) {
      Complex_Selector* lhs_tail = lhs->tail();
      Complex_Selector* rhs_tail = rhs->tail();
      if (lhs_tail->combinator() != rhs_tail->combinator()) return false;
      if (lhs_tail->head() && !rhs_tail->head()) return false;
      if (!lhs_tail->head() && rhs_tail->head()) return false;
      if (lhs_tail->head() && lhs_tail->head()) {
        if (!lhs_tail->head()->is_superselector_of(rhs_tail->head())) return false;
      }
    }

    bool found = false;
    Complex_Selector* marker = rhs;
    for (size_t i = 0, L = rhs->length(); i < L; ++i) {
      if (i == L-1)
      { return false; }
      if (lhs->head() && marker->head() && lhs->head()->is_superselector_of(marker->head(), wrapping))
      { found = true; break; }
      marker = marker->tail();
    }
    if (!found)
    { return false; }

    /*
      Hmm, I hope I have the logic right:

      if lhs has a combinator:
        if !(marker has a combinator) return false
        if !(lhs.combinator == '~' ? marker.combinator != '>' : lhs.combinator == marker.combinator) return false
        return lhs.tail-without-innermost.is_superselector_of(marker.tail-without-innermost)
      else if marker has a combinator:
        if !(marker.combinator == ">") return false
        return lhs.tail.is_superselector_of(marker.tail)
      else
        return lhs.tail.is_superselector_of(marker.tail)
    */
    if (lhs->combinator() != Complex_Selector::ANCESTOR_OF)
    {
      if (marker->combinator() == Complex_Selector::ANCESTOR_OF)
      { return false; }
      if (!(lhs->combinator() == Complex_Selector::PRECEDES ? marker->combinator() != Complex_Selector::PARENT_OF : lhs->combinator() == marker->combinator()))
      { return false; }
      return lhs->tail()->is_superselector_of(marker->tail());
    }
    else if (marker->combinator() != Complex_Selector::ANCESTOR_OF)
    {
      if (marker->combinator() != Complex_Selector::PARENT_OF)
      { return false; }
      return lhs->tail()->is_superselector_of(marker->tail());
    }
    else
    {
      return lhs->tail()->is_superselector_of(marker->tail());
    }
    // catch-all
    return false;
  }

  size_t Complex_Selector::length() const
  {
    // TODO: make this iterative
    if (!tail()) return 1;
    return 1 + tail()->length();
  }

  Complex_Selector* Complex_Selector::context(Context& ctx)
  {
    if (!tail()) return 0;
    if (!head()) return tail()->context(ctx);
    Complex_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, pstate(), combinator(), head(), tail()->context(ctx));
    cpy->media_block(media_block());
    return cpy;
  }

  // append another complex selector at the end
  // check if we need to append some headers
  // then we need to check for the combinator
  // only then we can safely set the new tail
  void Complex_Selector::append(Context& ctx, Complex_Selector* ss)
  {

    Complex_Selector* t = ss->tail();
    Combinator c = ss->combinator();
    String* r = ss->reference();
    Compound_Selector* h = ss->head();

    if (ss->has_line_feed()) has_line_feed(true);
    if (ss->has_line_break()) has_line_break(true);

    // append old headers
    if (h && h->length()) {
      if (last()->combinator() != ANCESTOR_OF && c != ANCESTOR_OF) {
        error("Invalid parent selector", pstate_);
      } else if (last()->head_ && last()->head_->length()) {
        Compound_Selector* rh = last()->head();
        size_t i = 0, L = h->length();
        if (dynamic_cast<Type_Selector*>(h->first())) {
          if (Selector_Qualifier* sq = dynamic_cast<Selector_Qualifier*>(rh->last())) {
            Selector_Qualifier* sqs = new Selector_Qualifier(*sq);
            sqs->name(sqs->name() + (*h)[0]->name());
            (*rh)[rh->length()-1] = sqs;
            for (i = 1; i < L; ++i) *rh << (*h)[i];
          } else if (Type_Selector* ts = dynamic_cast<Type_Selector*>(rh->last())) {
            Type_Selector* tss = new Type_Selector(*ts);
            tss->name(tss->name() + (*h)[0]->name());
            (*rh)[rh->length()-1] = tss;
            for (i = 1; i < L; ++i) *rh << (*h)[i];
          } else {
            *last()->head_ += h;
          }
        } else {
          *last()->head_ += h;
        }
      } else {
        *last()->head_ += h;
      }
    } else {
      // std::cerr << "has no or empty head\n";
    }

    if (last()) {
      if (last()->combinator() != ANCESTOR_OF && c != ANCESTOR_OF) {
        Complex_Selector* inter = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, pstate());
        inter->reference(r);
        inter->combinator(c);
        inter->tail(t);
        last()->tail(inter);
      } else {
        if (last()->combinator() == ANCESTOR_OF) {
          last()->combinator(c);
          last()->reference(r);
        }
        last()->tail(t);
      }
    }


  }

  Selector_List* Selector_List::parentize(Selector_List* ps, Context& ctx)
  {
    Selector_List* ss = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
    for (size_t pi = 0, pL = ps->length(); pi < pL; ++pi) {
      Selector_List* list = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
      *list << (*ps)[pi];
      for (size_t si = 0, sL = this->length(); si < sL; ++si) {
        *ss += (*this)[si]->parentize(list, ctx);
      }
    }
    return ss;
  }

  Selector_List* Complex_Selector::parentize(Selector_List* parents, Context& ctx)
  {

    Complex_Selector* tail = this->tail();
    Compound_Selector* head = this->head();

    // first parentize the tail (which may return an expanded list)
    Selector_List* tails = tail ? tail->parentize(parents, ctx) : 0;

    if (head && head->length() > 0) {

      // we have a parent selector in a simple compound list
      // mix parent complex selector into the compound list
      if (dynamic_cast<Parent_Selector*>((*head)[0])) {
        if (parents && parents->length()) {
          Selector_List* retval = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
          if (tails && tails->length() > 0) {
            for (size_t n = 0, nL = tails->length(); n < nL; ++n) {
              for (size_t i = 0, iL = parents->length(); i < iL; ++i) {
                Complex_Selector* t = (*tails)[n];
                Complex_Selector* parent = (*parents)[i];
                Complex_Selector* s = parent->cloneFully(ctx);
                Complex_Selector* ss = this->clone(ctx);
                ss->tail(t ? t->clone(ctx) : 0);
                Compound_Selector* h = head_->clone(ctx);
                if (h->length()) h->erase(h->begin());
                ss->head(h->length() ? h : 0);
                s->append(ctx, ss);
                *retval << s;
              }
            }
          }
          // have no tails but parents
          // loop above is inside out
          else {
            for (size_t i = 0, iL = parents->length(); i < iL; ++i) {
              Complex_Selector* parent = (*parents)[i];
              Complex_Selector* s = parent->cloneFully(ctx);
              Complex_Selector* ss = this->clone(ctx);
              ss->tail(tail ? tail->clone(ctx) : 0);
              Compound_Selector* h = head_->clone(ctx);
              if (h->length()) h->erase(h->begin());
              ss->head(h->length() ? h : 0);
              // \/ IMO ruby sass bug \/
              ss->has_line_feed(false);
              s->append(ctx, ss);
              *retval << s;
            }
          }
          return retval;
        }
        // have no parent but some tails
        else {
          Selector_List* retval = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
          if (tails && tails->length() > 0) {
            for (size_t n = 0, nL = tails->length(); n < nL; ++n) {
              Complex_Selector* cpy = this->clone(ctx);
              cpy->tail((*tails)[n]->cloneFully(ctx));
              cpy->head(SASS_MEMORY_NEW(ctx.mem, Compound_Selector, head->pstate()));
              for (size_t i = 1, L = this->head()->length(); i < L; ++i)
                *cpy->head() << (*this->head())[i];
              if (!cpy->head()->length()) cpy->head(0);
              *retval << cpy->skip_empty_reference();
            }
          }
          // have no parent and not tails
          else {
            Complex_Selector* cpy = this->clone(ctx);
            cpy->head(SASS_MEMORY_NEW(ctx.mem, Compound_Selector, head->pstate()));
            for (size_t i = 1, L = this->head()->length(); i < L; ++i)
              *cpy->head() << (*this->head())[i];
            if (!cpy->head()->length()) cpy->head(0);
            *retval << cpy->skip_empty_reference();
          }
          return retval;
        }
      }
      // no parent selector in head
      else {
        return this->tails(ctx, tails);
      }

    }
    // has no head
    else {
      return this->tails(ctx, tails);
    }

    // unreachable
    return 0;
  }

  Selector_List* Complex_Selector::tails(Context& ctx, Selector_List* tails)
  {
    Selector_List* rv = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate_);
    if (tails && tails->length()) {
      for (size_t i = 0, iL = tails->length(); i < iL; ++i) {
        Complex_Selector* pr = this->clone(ctx);
        pr->tail((*tails)[i]);
        *rv << pr;
      }
    }
    else {
      *rv << this;
    }
    return rv;
  }

  // return the last tail that is defined
  Complex_Selector* Complex_Selector::first()
  {
    // declare variables used in loop
    Complex_Selector* cur = this->tail_;
    const Compound_Selector* head = head_;
    // processing loop
    while (cur)
    {
      // get the head
      head = cur->head_;
      // check for single parent ref
      if (head && head->length() == 1)
      {
        // abort (and return) if it is not a parent selector
        if (!dynamic_cast<Parent_Selector*>((*head)[0])) break;
      }
      // advance to next
      cur = cur->tail_;
    }
    // result
    return cur;
  }

  // return the last tail that is defined
  const Complex_Selector* Complex_Selector::first() const
  {
    // declare variables used in loop
    const Complex_Selector* cur = this->tail_;
    const Compound_Selector* head = head_;
    // processing loop
    while (cur)
    {
      // get the head
      head = cur->head_;
      // check for single parent ref
      if (head && head->length() == 1)
      {
        // abort (and return) if it is not a parent selector
        if (!dynamic_cast<Parent_Selector*>((*head)[0])) break;
      }
      // advance to next
      cur = cur->tail_;
    }
    // result
    return cur;
  }

  // return the last tail that is defined
  Complex_Selector* Complex_Selector::last()
  {
    // ToDo: implement with a while loop
    return tail_? tail_->last() : this;
  }

  // return the last tail that is defined
  const Complex_Selector* Complex_Selector::last() const
  {
    // ToDo: implement with a while loop
    return tail_? tail_->last() : this;
  }


  Complex_Selector::Combinator Complex_Selector::clear_innermost()
  {
    Combinator c;
    if (!tail() || tail()->tail() == 0)
    { c = combinator(); combinator(ANCESTOR_OF); tail(0); }
    else
    { c = tail()->clear_innermost(); }
    return c;
  }

  void Complex_Selector::set_innermost(Complex_Selector* val, Combinator c)
  {
    if (!tail())
    { tail(val); combinator(c); }
    else
    { tail()->set_innermost(val, c); }
  }

  Complex_Selector* Complex_Selector::clone(Context& ctx) const
  {
    Complex_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, *this);
    cpy->media_block(this->media_block());
    if (tail()) cpy->tail(tail()->clone(ctx));
    return cpy;
  }

  Complex_Selector* Complex_Selector::cloneFully(Context& ctx) const
  {
    Complex_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Complex_Selector, *this);

    if (head()) {
      cpy->head(head()->clone(ctx));
    }

    if (tail()) {
      cpy->tail(tail()->cloneFully(ctx));
    }

    return cpy;
  }

  Compound_Selector* Compound_Selector::clone(Context& ctx) const
  {
    Compound_Selector* cpy = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, *this);
    cpy->media_block(this->media_block());
    return cpy;
  }

  Selector_List* Selector_List::clone(Context& ctx) const
  {
    Selector_List* cpy = SASS_MEMORY_NEW(ctx.mem, Selector_List, *this);
    cpy->media_block(this->media_block());
    return cpy;
  }

  Selector_List* Selector_List::cloneFully(Context& ctx) const
  {
    Selector_List* cpy = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
    for (size_t i = 0, L = length(); i < L; ++i) {
      *cpy << (*this)[i]->cloneFully(ctx);
    }
    return cpy;
  }

  /* not used anymore - remove?
  Selector_Placeholder* Selector::find_placeholder()
  {
    return 0;
  }*/

  // remove parent selector references
  // basically unwraps parsed selectors
  void Selector_List::remove_parent_selectors()
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = length(); i < L; ++i) {
      if (!(*this)[i]->head()) continue;
      if ((*this)[i]->head()->is_empty_reference()) {
        // simply move to the next tail if we have "no" combinator
        if ((*this)[i]->combinator() == Complex_Selector::ANCESTOR_OF) {
          if ((*this)[i]->tail() && (*this)[i]->has_line_feed()) {
            (*this)[i]->tail()->has_line_feed(true);
          }
          (*this)[i] = (*this)[i]->tail();
        }
        // otherwise remove the first item from head
        else {
          (*this)[i]->head()->erase((*this)[i]->head()->begin());
        }
      }
    }
  }

  void Selector_List::adjust_after_pushing(Complex_Selector* c)
  {
    if (c->has_reference())   has_reference(true);
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Complex_Selector::is_superselector_of(Selector_List *sub, std::string wrapping)
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Selector_List::is_superselector_of(Selector_List *sub, std::string wrapping)
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Compound_Selector *sub, std::string wrapping)
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub, wrapping)) return true;
    }
    return false;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Complex_Selector *sub, std::string wrapping)
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub)) return true;
    }
    return false;
  }

  Selector_List* Selector_List::unify_with(Selector_List* rhs, Context& ctx) {
    std::vector<Complex_Selector*> unified_complex_selectors;
    // Unify all of children with RHS's children, storing the results in `unified_complex_selectors`
    for (size_t lhs_i = 0, lhs_L = length(); lhs_i < lhs_L; ++lhs_i) {
      Complex_Selector* seq1 = (*this)[lhs_i];
      for(size_t rhs_i = 0, rhs_L = rhs->length(); rhs_i < rhs_L; ++rhs_i) {
        Complex_Selector* seq2 = (*rhs)[rhs_i];

        Selector_List* result = seq1->unify_with(seq2, ctx);
        if( result ) {
          for(size_t i = 0, L = result->length(); i < L; ++i) {
            unified_complex_selectors.push_back( (*result)[i] );
          }
        }
      }
    }

    // Creates the final Selector_List by combining all the complex selectors
    Selector_List* final_result = SASS_MEMORY_NEW(ctx.mem, Selector_List, pstate());
    for (auto itr = unified_complex_selectors.begin(); itr != unified_complex_selectors.end(); ++itr) {
      *final_result << *itr;
    }
    return final_result;
  }

  void Selector_List::populate_extends(Selector_List* extendee, Context& ctx, ExtensionSubsetMap& extends) {
    To_String to_string;

    Selector_List* extender = this;
    for (auto complex_sel : extendee->elements()) {
      Complex_Selector* c = complex_sel;


      // Ignore any parent selectors, until we find the first non Selector_Reference head
      Compound_Selector* compound_sel = c->head();
      Complex_Selector* pIter = complex_sel;
      while (pIter) {
        Compound_Selector* pHead = pIter->head();
        if (pHead && dynamic_cast<Parent_Selector*>(pHead->elements()[0]) == NULL) {
          compound_sel = pHead;
          break;
        }

        pIter = pIter->tail();
      }

      if (!pIter->head() || pIter->tail()) {
        error("nested selectors may not be extended", c->pstate());
      }

      compound_sel->is_optional(extendee->is_optional());

      for (size_t i = 0, L = extender->length(); i < L; ++i) {
        extends.put(compound_sel->to_str_vec(), std::make_pair((*extender)[i], compound_sel));
      }
    }
  };

  std::vector<std::string> Compound_Selector::to_str_vec()
  {
    To_String to_string;
    std::vector<std::string> result;
    result.reserve(length());
    for (size_t i = 0, L = length(); i < L; ++i)
    { result.push_back((*this)[i]->perform(&to_string)); }
    return result;
  }

  Compound_Selector* Compound_Selector::minus(Compound_Selector* rhs, Context& ctx)
  {
    To_String to_string(&ctx);
    Compound_Selector* result = SASS_MEMORY_NEW(ctx.mem, Compound_Selector, pstate());
    // result->has_parent_reference(has_parent_reference());

    // not very efficient because it needs to preserve order
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      bool found = false;
      std::string thisSelector((*this)[i]->perform(&to_string));
      for (size_t j = 0, M = rhs->length(); j < M; ++j)
      {
        if (thisSelector == (*rhs)[j]->perform(&to_string))
        {
          found = true;
          break;
        }
      }
      if (!found) (*result) << (*this)[i];
    }

    return result;
  }

  void Compound_Selector::mergeSources(SourcesSet& sources, Context& ctx)
  {
    for (SourcesSet::iterator iterator = sources.begin(), endIterator = sources.end(); iterator != endIterator; ++iterator) {
      this->sources_.insert((*iterator)->clone(ctx));
    }
  }

  void Arguments::adjust_after_pushing(Argument* a)
  {
    if (!a->name().empty()) {
      if (/* has_rest_argument_ || */ has_keyword_argument_) {
        error("named arguments must precede variable-length argument", a->pstate());
      }
      has_named_arguments_ = true;
    }
    else if (a->is_rest_argument()) {
      if (has_rest_argument_) {
        error("functions and mixins may only be called with one variable-length argument", a->pstate());
      }
      if (has_keyword_argument_) {
        error("only keyword arguments may follow variable arguments", a->pstate());
      }
      has_rest_argument_ = true;
    }
    else if (a->is_keyword_argument()) {
      if (has_keyword_argument_) {
        error("functions and mixins may only be called with one keyword argument", a->pstate());
      }
      has_keyword_argument_ = true;
    }
    else {
      if (has_rest_argument_) {
        error("ordinal arguments must precede variable-length arguments", a->pstate());
      }
      if (has_named_arguments_) {
        error("ordinal arguments must precede named arguments", a->pstate());
      }
    }
  }

  bool Ruleset::is_invisible() const {
    Selector_List* sl = static_cast<Selector_List*>(selector());
    for (size_t i = 0, L = sl->length(); i < L; ++i)
      if (!(*sl)[i]->has_placeholder()) return false;
    return true;
  }

  bool Media_Block::is_invisible() const {
    for (size_t i = 0, L = block()->length(); i < L; ++i) {
      if (!(*block())[i]->is_invisible()) return false;
    }
    return true;
  }

  Number::Number(ParserState pstate, double val, std::string u, bool zero)
  : Value(pstate),
    value_(val),
    zero_(zero),
    numerator_units_(std::vector<std::string>()),
    denominator_units_(std::vector<std::string>()),
    hash_(0)
  {
    size_t l = 0, r = 0;
    if (!u.empty()) {
      bool nominator = true;
      while (true) {
        r = u.find_first_of("*/", l);
        std::string unit(u.substr(l, r == std::string::npos ? r : r - l));
        if (!unit.empty()) {
          if (nominator) numerator_units_.push_back(unit);
          else denominator_units_.push_back(unit);
        }
        if (r == std::string::npos) break;
        // ToDo: should error for multiple slashes
        // if (!nominator && u[r] == '/') error(...)
        if (u[r] == '/')
          nominator = false;
        l = r + 1;
      }
    }
    concrete_type(NUMBER);
  }

  std::string Number::unit() const
  {
    std::string u;
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) {
      if (i) u += '*';
      u += numerator_units_[i];
    }
    if (!denominator_units_.empty()) u += '/';
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) {
      if (i) u += '*';
      u += denominator_units_[i];
    }
    return u;
  }

  bool Number::is_unitless()
  { return numerator_units_.empty() && denominator_units_.empty(); }

  void Number::normalize(const std::string& prefered, bool strict)
  {

    // first make sure same units cancel each other out
    // it seems that a map table will fit nicely to do this
    // we basically construct exponents for each unit
    // has the advantage that they will be pre-sorted
    std::map<std::string, int> exponents;

    // initialize by summing up occurences in unit vectors
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) ++ exponents[numerator_units_[i]];
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) -- exponents[denominator_units_[i]];

    // the final conversion factor
    double factor = 1;

    // get the first entry of numerators
    // forward it when entry is converted
    std::vector<std::string>::iterator nom_it = numerator_units_.begin();
    std::vector<std::string>::iterator nom_end = numerator_units_.end();
    std::vector<std::string>::iterator denom_it = denominator_units_.begin();
    std::vector<std::string>::iterator denom_end = denominator_units_.end();

    // main normalization loop
    // should be close to optimal
    while (denom_it != denom_end)
    {
      // get and increment afterwards
      const std::string denom = *(denom_it ++);
      // skip already canceled out unit
      if (exponents[denom] >= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(denom) == UNKNOWN) continue;
      // now search for nominator
      while (nom_it != nom_end)
      {
        // get and increment afterwards
        const std::string nom = *(nom_it ++);
        // skip already canceled out unit
        if (exponents[nom] <= 0) continue;
        // skip all units we don't know how to convert
        if (string_to_unit(nom) == UNKNOWN) continue;
        // we now have two convertable units
        // add factor for current conversion
        factor *= conversion_factor(nom, denom, strict);
        // update nominator/denominator exponent
        -- exponents[nom]; ++ exponents[denom];
        // inner loop done
        break;
      }
    }

    // now we can build up the new unit arrays
    numerator_units_.clear();
    denominator_units_.clear();

    // build them by iterating over the exponents
    for (auto exp : exponents)
    {
      // maybe there is more effecient way to push
      // the same item multiple times to a vector?
      for(size_t i = 0, S = abs(exp.second); i < S; ++i)
      {
        // opted to have these switches in the inner loop
        // makes it more readable and should not cost much
        if (!exp.first.empty()) {
          if (exp.second < 0) denominator_units_.push_back(exp.first);
          else if (exp.second > 0) numerator_units_.push_back(exp.first);
        }
      }
    }

    // apply factor to value_
    // best precision this way
    value_ *= factor;

    // maybe convert to other unit
    // easier implemented on its own
    try { convert(prefered, strict); }
    catch (incompatibleUnits& err)
    { error(err.what(), pstate()); }
    catch (...) { throw; }

  }

  void Number::convert(const std::string& prefered, bool strict)
  {
    // abort if unit is empty
    if (prefered.empty()) return;

    // first make sure same units cancel each other out
    // it seems that a map table will fit nicely to do this
    // we basically construct exponents for each unit
    // has the advantage that they will be pre-sorted
    std::map<std::string, int> exponents;

    // initialize by summing up occurences in unit vectors
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) ++ exponents[numerator_units_[i]];
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) -- exponents[denominator_units_[i]];

    // the final conversion factor
    double factor = 1;

    std::vector<std::string>::iterator denom_it = denominator_units_.begin();
    std::vector<std::string>::iterator denom_end = denominator_units_.end();

    // main normalization loop
    // should be close to optimal
    while (denom_it != denom_end)
    {
      // get and increment afterwards
      const std::string denom = *(denom_it ++);
      // check if conversion is needed
      if (denom == prefered) continue;
      // skip already canceled out unit
      if (exponents[denom] >= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(denom) == UNKNOWN) continue;
      // we now have two convertable units
      // add factor for current conversion
      factor *= conversion_factor(denom, prefered, strict);
      // update nominator/denominator exponent
      ++ exponents[denom]; -- exponents[prefered];
    }

    std::vector<std::string>::iterator nom_it = numerator_units_.begin();
    std::vector<std::string>::iterator nom_end = numerator_units_.end();

    // now search for nominator
    while (nom_it != nom_end)
    {
      // get and increment afterwards
      const std::string nom = *(nom_it ++);
      // check if conversion is needed
      if (nom == prefered) continue;
      // skip already canceled out unit
      if (exponents[nom] <= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(nom) == UNKNOWN) continue;
      // we now have two convertable units
      // add factor for current conversion
      factor *= conversion_factor(nom, prefered, strict);
      // update nominator/denominator exponent
      -- exponents[nom]; ++ exponents[prefered];
    }

    // now we can build up the new unit arrays
    numerator_units_.clear();
    denominator_units_.clear();

    // build them by iterating over the exponents
    for (auto exp : exponents)
    {
      // maybe there is more effecient way to push
      // the same item multiple times to a vector?
      for(size_t i = 0, S = abs(exp.second); i < S; ++i)
      {
        // opted to have these switches in the inner loop
        // makes it more readable and should not cost much
        if (!exp.first.empty()) {
          if (exp.second < 0) denominator_units_.push_back(exp.first);
          else if (exp.second > 0) numerator_units_.push_back(exp.first);
        }
      }
    }

    // apply factor to value_
    // best precision this way
    value_ *= factor;

  }

  // useful for making one number compatible with another
  std::string Number::find_convertible_unit() const
  {
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) {
      std::string u(numerator_units_[i]);
      if (string_to_unit(u) != UNKNOWN) return u;
    }
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) {
      std::string u(denominator_units_[i]);
      if (string_to_unit(u) != UNKNOWN) return u;
    }
    return std::string();
  }

  bool Custom_Warning::operator== (const Expression& rhs) const
  {
    if (const Custom_Warning* r = dynamic_cast<const Custom_Warning*>(&rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Custom_Error::operator== (const Expression& rhs) const
  {
    if (const Custom_Error* r = dynamic_cast<const Custom_Error*>(&rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Number::operator== (const Expression& rhs) const
  {
    if (const Number* r = dynamic_cast<const Number*>(&rhs)) {
      return (numerator_units_ == r->numerator_units_) &&
             (denominator_units_ == r->denominator_units_) &&
             std::fabs(value() - r->value()) < NUMBER_EPSILON;
    }
    return false;
  }

  bool Number::operator< (const Number& rhs) const
  {
    Number tmp_r(rhs);
    tmp_r.normalize(find_convertible_unit());
    std::string l_unit(unit());
    std::string r_unit(tmp_r.unit());
    if (!l_unit.empty() && !r_unit.empty() && unit() != tmp_r.unit()) {
      error("cannot compare numbers with incompatible units", pstate());
    }
    return value() < tmp_r.value();
  }

  bool String_Quoted::operator== (const Expression& rhs) const
  {
    if (const String_Quoted* qstr = dynamic_cast<const String_Quoted*>(&rhs)) {
      return (value() == qstr->value());
    } else if (const String_Constant* cstr = dynamic_cast<const String_Constant*>(&rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Constant::operator== (const Expression& rhs) const
  {
    if (const String_Quoted* qstr = dynamic_cast<const String_Quoted*>(&rhs)) {
      return (value() == qstr->value());
    } else if (const String_Constant* cstr = dynamic_cast<const String_Constant*>(&rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Schema::operator== (const Expression& rhs) const
  {
    if (const String_Schema* r = dynamic_cast<const String_Schema*>(&rhs)) {
      if (length() != r->length()) return false;
      for (size_t i = 0, L = length(); i < L; ++i) {
        Expression* rv = (*r)[i];
        Expression* lv = (*this)[i];
        if (!lv || !rv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Boolean::operator== (const Expression& rhs) const
  {
    if (const Boolean* r = dynamic_cast<const Boolean*>(&rhs)) {
      return (value() == r->value());
    }
    return false;
  }

  bool Color::operator== (const Expression& rhs) const
  {
    if (const Color* r = dynamic_cast<const Color*>(&rhs)) {
      return r_ == r->r() &&
             g_ == r->g() &&
             b_ == r->b() &&
             a_ == r->a();
    }
    return false;
  }

  bool List::operator== (const Expression& rhs) const
  {
    if (const List* r = dynamic_cast<const List*>(&rhs)) {
      if (length() != r->length()) return false;
      if (separator() != r->separator()) return false;
      for (size_t i = 0, L = length(); i < L; ++i) {
        Expression* rv = (*r)[i];
        Expression* lv = (*this)[i];
        if (!lv || !rv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Map::operator== (const Expression& rhs) const
  {
    if (const Map* r = dynamic_cast<const Map*>(&rhs)) {
      if (length() != r->length()) return false;
      for (auto key : keys()) {
        Expression* lv = at(key);
        Expression* rv = r->at(key);
        if (!rv || !lv) return false;
        if (!(*lv == *rv)) return false;
      }
      return true;
    }
    return false;
  }

  bool Null::operator== (const Expression& rhs) const
  {
    return rhs.concrete_type() == NULL_VAL;
  }

  size_t List::size() const {
    if (!is_arglist_) return length();
    // arglist expects a list of arguments
    // so we need to break before keywords
    for (size_t i = 0, L = length(); i < L; ++i) {
      if (Argument* arg = dynamic_cast<Argument*>((*this)[i])) {
        if (!arg->name().empty()) return i;
      }
    }
    return length();
  }

  Expression* Hashed::at(Expression* k) const
  {
    if (elements_.count(k))
    { return elements_.at(k); }
    else { return &sass_null; }
  }

  std::string Map::to_string(bool compressed, int precision) const
  {
    std::string res("");
    if (empty()) return res;
    if (is_invisible()) return res;
    bool items_output = false;
    for (auto key : keys()) {
      if (key->is_invisible()) continue;
      if (at(key)->is_invisible()) continue;
      if (items_output) res += compressed ? "," : ", ";
      Value* v_key = dynamic_cast<Value*>(key);
      Value* v_val = dynamic_cast<Value*>(at(key));
      if (v_key) res += v_key->to_string(compressed, precision);
      res += compressed ? ":" : ": ";
      if (v_val) res += v_val->to_string(compressed, precision);
      items_output = true;
    }
    return res;
  }

  std::string List::to_string(bool compressed, int precision) const
  {
    std::string res("");
    if (empty()) return res;
    if (is_invisible()) return res;
    bool items_output = false;
    std::string sep = separator() == SASS_COMMA ? "," : " ";
    if (!compressed && sep == ",") sep += " ";
    for (size_t i = 0, L = size(); i < L; ++i) {
      Expression* item = (*this)[i];
      if (item->is_invisible()) continue;
      if (items_output) res += sep;
      if (Value* v_val = dynamic_cast<Value*>(item))
      { res += v_val->to_string(compressed, precision); }
      items_output = true;
    }
    return res;
  }

  std::string String_Schema::to_string(bool compressed, int precision) const
  {
    std::string res("");
    for (size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_interpolant()) res += "#{";
      if (Value* val = dynamic_cast<Value*>((*this)[i]))
      { res += val->to_string(compressed, precision); }
      if ((*this)[i]->is_interpolant()) res += "}";
    }
    return res;
  }

  std::string Null::to_string(bool compressed, int precision) const
  {
    return "null";
  }

  std::string Boolean::to_string(bool compressed, int precision) const
  {
    return value_ ? "true" : "false";
  }

  // helper function for serializing colors
  template <size_t range>
  static double cap_channel(double c) {
    if      (c > range) return range;
    else if (c < 0)     return 0;
    else                return c;
  }

  std::string Color::to_string(bool compressed, int precision) const
  {
    std::stringstream ss;

    // original color name
    // maybe an unknown token
    std::string name = disp();

    // resolved color
    std::string res_name = name;

    double r = round(cap_channel<0xff>(r_));
    double g = round(cap_channel<0xff>(g_));
    double b = round(cap_channel<0xff>(b_));
    double a = cap_channel<1>   (a_);

    // get color from given name (if one was given at all)
    if (name != "" && name_to_color(name)) {
      const Color* n = name_to_color(name);
      r = round(cap_channel<0xff>(n->r()));
      g = round(cap_channel<0xff>(n->g()));
      b = round(cap_channel<0xff>(n->b()));
      a = cap_channel<1>   (n->a());
    }
    // otherwise get the possible resolved color name
    else {
      double numval = r * 0x10000 + g * 0x100 + b;
      if (color_to_name(numval))
        res_name = color_to_name(numval);
    }

    std::stringstream hexlet;
    hexlet << '#' << std::setw(1) << std::setfill('0');
    // create a short color hexlet if there is any need for it
    if (compressed && is_color_doublet(r, g, b) && a == 1) {
      hexlet << std::hex << std::setw(1) << (static_cast<unsigned long>(r) >> 4);
      hexlet << std::hex << std::setw(1) << (static_cast<unsigned long>(g) >> 4);
      hexlet << std::hex << std::setw(1) << (static_cast<unsigned long>(b) >> 4);
    } else {
      hexlet << std::hex << std::setw(2) << static_cast<unsigned long>(r);
      hexlet << std::hex << std::setw(2) << static_cast<unsigned long>(g);
      hexlet << std::hex << std::setw(2) << static_cast<unsigned long>(b);
    }

    if (compressed && !this->is_delayed()) name = "";

    // retain the originally specified color definition if unchanged
    if (name != "") {
      ss << name;
    }
    else if (r == 0 && g == 0 && b == 0 && a == 0) {
        ss << "transparent";
    }
    else if (a >= 1) {
      if (res_name != "") {
        if (compressed && hexlet.str().size() < res_name.size()) {
          ss << hexlet.str();
        } else {
          ss << res_name;
        }
      }
      else {
        ss << hexlet.str();
      }
    }
    else {
      ss << "rgba(";
      ss << static_cast<unsigned long>(r) << ",";
      if (!compressed) ss << " ";
      ss << static_cast<unsigned long>(g) << ",";
      if (!compressed) ss << " ";
      ss << static_cast<unsigned long>(b) << ",";
      if (!compressed) ss << " ";
      ss << a << ')';
    }

    return ss.str();

  }

  std::string Number::to_string(bool compressed, int precision) const
  {

    std::string res;

    // check if the fractional part of the value equals to zero
    // neat trick from http://stackoverflow.com/a/1521682/1550314
    // double int_part; bool is_int = modf(value, &int_part) == 0.0;

    // this all cannot be done with one run only, since fixed
    // output differs from normal output and regular output
    // can contain scientific notation which we do not want!

    // first sample
    std::stringstream ss;
    ss.precision(12);
    ss << value_;

    // check if we got scientific notation in result
    if (ss.str().find_first_of("e") != std::string::npos) {
      ss.clear(); ss.str(std::string());
      ss.precision(std::max(12, precision));
      ss << std::fixed << value_;
    }

    std::string tmp = ss.str();
    size_t pos_point = tmp.find_first_of(".,");
    size_t pos_fract = tmp.find_last_not_of("0");
    bool is_int = pos_point == pos_fract ||
                  pos_point == std::string::npos;

    // reset stream for another run
    ss.clear(); ss.str(std::string());

    // take a shortcut for integers
    if (is_int)
    {
      ss.precision(0);
      ss << std::fixed << value_;
      res = std::string(ss.str());
    }
    // process floats
    else
    {
      // do we have have too much precision?
      if (pos_fract < precision + pos_point)
      { precision = (int)(pos_fract - pos_point); }
      // round value again
      ss.precision(precision);
      ss << std::fixed << value_;
      res = std::string(ss.str());
      // maybe we truncated up to decimal point
      size_t pos = res.find_last_not_of("0");
      bool at_dec_point = res[pos] == '.' ||
                          res[pos] == ',';
      // don't leave a blank point
      if (at_dec_point) ++ pos;
      res.resize (pos + 1);
    }

    // some final cosmetics
    if (res == "-0.0") res.erase(0, 1);
    else if (res == "-0") res.erase(0, 1);
    else if (res == "") res = "0";

    // add unit now
    res += unit();

    // and return
    return res;

  }

  std::string String_Quoted::to_string(bool compressed, int precision) const
  {
    return quote_mark_ ? quote(value_, quote_mark_, true) : value_;
  }

  std::string String_Constant::to_string(bool compressed, int precision) const
  {
    return quote_mark_ ? quote(value_, quote_mark_, true) : value_;
  }

  std::string Custom_Error::to_string(bool compressed, int precision) const
  {
    return message();
  }
  std::string Custom_Warning::to_string(bool compressed, int precision) const
  {
    return message();
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Additional method on Lists to retrieve values directly or from an encompassed Argument.
  //////////////////////////////////////////////////////////////////////////////////////////
  Expression* List::value_at_index(size_t i) {
    if (is_arglist_) {
      if (Argument* arg = dynamic_cast<Argument*>((*this)[i])) {
        return arg->value();
      } else {
        return (*this)[i];
      }
    } else {
      return (*this)[i];
    }
  }

}

