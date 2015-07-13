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
  using namespace std;

  static Null sass_null(Sass::Null(ParserState("null")));

  void AST_Node::update_pstate(const ParserState& pstate)
  {
    pstate_.offset += pstate - pstate_ + pstate.offset;
  }


  bool Compound_Selector::operator<(const Compound_Selector& rhs) const
  {
    To_String to_string;
    // ugly
    return const_cast<Compound_Selector*>(this)->perform(&to_string) <
           const_cast<Compound_Selector&>(rhs).perform(&to_string);
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

  bool Complex_Selector::operator<(const Complex_Selector& rhs) const
  {
    To_String to_string;
    return const_cast<Complex_Selector*>(this)->perform(&to_string) <
           const_cast<Complex_Selector&>(rhs).perform(&to_string);
  }

  bool Complex_Selector::operator==(const Complex_Selector& rhs) const {
    // TODO: We have to access the tail directly using tail_ since ADD_PROPERTY doesn't provide a const version.

    const Complex_Selector* pOne = this;
    const Complex_Selector* pTwo = &rhs;

    // Consume any empty references at the beginning of the Complex_Selector
    if (pOne->combinator() == Complex_Selector::ANCESTOR_OF && pOne->head()->is_empty_reference()) {
      pOne = pOne->tail_;
    }
    if (pTwo->combinator() == Complex_Selector::ANCESTOR_OF && pTwo->head()->is_empty_reference()) {
      pTwo = pTwo->tail_;
    }

    while (pOne && pTwo) {
      if (pOne->combinator() != pTwo->combinator()) {
        return false;
      }

      if (*(pOne->head()) != *(pTwo->head())) {
        return false;
      }

      pOne = pOne->tail_;
      pTwo = pTwo->tail_;
    }

    return pOne == NULL && pTwo == NULL;
  }

  Compound_Selector* Compound_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    Compound_Selector* unified = rhs;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if (!unified) break;
      else          unified = (*this)[i]->unify_with(unified, ctx);
    }
    return unified;
  }

  bool Simple_Selector::operator==(const Simple_Selector& rhs) const
  {
    // Compare the string representations for equality.

    // Cast away const here. To_String should take a const object, but it doesn't.
    Simple_Selector* pLHS = const_cast<Simple_Selector*>(this);
    Simple_Selector* pRHS = const_cast<Simple_Selector*>(&rhs);

    To_String to_string;
    return pLHS->perform(&to_string) == pRHS->perform(&to_string);
  }

  bool Simple_Selector::operator<(const Simple_Selector& rhs) const {
    // Use the string representation for ordering.

    // Cast away const here. To_String should take a const object, but it doesn't.
    Simple_Selector* pLHS = const_cast<Simple_Selector*>(this);
    Simple_Selector* pRHS = const_cast<Simple_Selector*>(&rhs);

    To_String to_string;
    return pLHS->perform(&to_string) < pRHS->perform(&to_string);
  }

  Compound_Selector* Simple_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    To_String to_string(&ctx);
    for (size_t i = 0, L = rhs->length(); i < L; ++i)
    { if (perform(&to_string) == (*rhs)[i]->perform(&to_string)) return rhs; }

    // check for pseudo elements because they need to come last
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
      Compound_Selector* cpy = new (ctx.mem) Compound_Selector(*rhs);
      (*cpy) << this;
      return cpy;
    }
    Compound_Selector* cpy = new (ctx.mem) Compound_Selector(rhs->pstate());
    for (size_t j = 0; j < i; ++j)
    { (*cpy) << (*rhs)[j]; }
    (*cpy) << this;
    for (size_t j = i; j < L; ++j)
    { (*cpy) << (*rhs)[j]; }
    return cpy;
  }

  Compound_Selector* Type_Selector::unify_with(Compound_Selector* rhs, Context& ctx)
  {
    // TODO: handle namespaces

    // if the rhs is empty, just return a copy of this
    if (rhs->length() == 0) {
      Compound_Selector* cpy = new (ctx.mem) Compound_Selector(rhs->pstate());
      (*cpy) << this;
      return cpy;
    }

    // if this is a universal selector and rhs is not empty, just return the rhs
    if (name() == "*")
    { return new (ctx.mem) Compound_Selector(*rhs); }


    Simple_Selector* rhs_0 = (*rhs)[0];
    // otherwise, this is a tag name
    if (typeid(*rhs_0) == typeid(Type_Selector))
    {
      // if rhs is universal, just return this tagname + rhs's qualifiers
      if (static_cast<Type_Selector*>(rhs_0)->name() == "*")
      {
        Compound_Selector* cpy = new (ctx.mem) Compound_Selector(rhs->pstate());
        (*cpy) << this;
        for (size_t i = 1, L = rhs->length(); i < L; ++i)
        { (*cpy) << (*rhs)[i]; }
        return cpy;
      }
      // if rhs is another tag name and it matches this, return rhs
      else if (static_cast<Type_Selector*>(rhs_0)->name() == name())
      { return new (ctx.mem) Compound_Selector(*rhs); }
      // else the tag names don't match; return nil
      else
      { return 0; }
    }
    // else it's a tag name and a bunch of qualifiers -- just append them
    Compound_Selector* cpy = new (ctx.mem) Compound_Selector(rhs->pstate());
    (*cpy) << this;
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

  bool Compound_Selector::is_superselector_of(Selector_List* rhs, string wrapped)
  {
    for (Complex_Selector* item : rhs->elements()) {
      if (is_superselector_of(item, wrapped)) return true;
    }
    return false;
  }

  bool Compound_Selector::is_superselector_of(Complex_Selector* rhs, string wrapped)
  {
    if (rhs->head()) return is_superselector_of(rhs->head(), wrapped);
    return false;
  }

  bool Compound_Selector::is_superselector_of(Compound_Selector* rhs, string wrapping)
  {
    To_String to_string;

    Compound_Selector* lhs = this;
    Simple_Selector* lbase = lhs->base();
    Simple_Selector* rbase = rhs->base();

    // Check if pseudo-elements are the same between the selectors

    set<string> lpsuedoset, rpsuedoset;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if ((*this)[i]->is_pseudo_element()) {
        string pseudo((*this)[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        lpsuedoset.insert(pseudo);
      }
    }
    for (size_t i = 0, L = rhs->length(); i < L; ++i)
    {
      if ((*rhs)[i]->is_pseudo_element()) {
        string pseudo((*rhs)[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        rpsuedoset.insert(pseudo);
      }
    }
    if (lpsuedoset != rpsuedoset) {
      return false;
    }

    set<string> lset, rset;

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
            throw runtime_error("wrapped not selector is not a list");
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

    if (lset.size() == 0) return true;
    // return true if rset contains all the elements of lset
    return includes(rset.begin(), rset.end(), lset.begin(), lset.end());

  }

  Selector_List* Complex_Selector::unify_with(Complex_Selector* other, Context& ctx) {
    To_String to_string;
    Compound_Selector* thisBase = last()->head();
    Compound_Selector* rhsBase = other->last()->head();


    if( thisBase == 0 || rhsBase == 0 ) return 0;

    // Not sure about this check, but closest way I could check to see if this is a ruby 'SimpleSequence' equivalent
    if(  tail()->combinator() != Combinator::ANCESTOR_OF || other->tail()->combinator() != Combinator::ANCESTOR_OF ) return 0;

    Compound_Selector* unified = rhsBase->unify_with(thisBase, ctx);
    if( unified == 0 ) return 0;

    Node lhsNode = complexSelectorToNode(this, ctx);
    Node rhsNode = complexSelectorToNode(other, ctx);

    // Create a temp Complex_Selector, turn it into a Node, and combine it with the existing RHS node
    Complex_Selector* fakeComplexSelector = new (ctx.mem) Complex_Selector(ParserState("[NODE]"), Complex_Selector::ANCESTOR_OF, unified, NULL);
    Node unifiedNode = complexSelectorToNode(fakeComplexSelector, ctx);
    rhsNode.plus(unifiedNode);

    Node node = Extend::subweave(lhsNode, rhsNode, ctx);

    Selector_List* result = new (ctx.mem) Selector_List(pstate());
    for (NodeDeque::iterator iter = node.collection()->begin(), iterEnd = node.collection()->end(); iter != iterEnd; iter++) {
      Node childNode = *iter;
      childNode = Node::naiveTrim(childNode, ctx);

      Complex_Selector* childNodeAsComplexSelector = nodeToComplexSelector(childNode, ctx);
      if( childNodeAsComplexSelector ) { (*result) << childNodeAsComplexSelector; }
    }
    return result->length() ? result : 0;
  }

  bool Compound_Selector::operator==(const Compound_Selector& rhs) const {
    To_String to_string;

    // Check if pseudo-elements are the same between the selectors

    set<string> lpsuedoset, rpsuedoset;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if ((*this)[i]->is_pseudo_element()) {
        string pseudo((*this)[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        lpsuedoset.insert(pseudo);
      }
    }
    for (size_t i = 0, L = rhs.length(); i < L; ++i)
    {
      if (rhs[i]->is_pseudo_element()) {
        string pseudo(rhs[i]->perform(&to_string));
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        rpsuedoset.insert(pseudo);
      }
    }
    if (lpsuedoset != rpsuedoset) {
      return false;
    }

    // Check the rest of the SimpleSelectors
    // Use string representations. We can't create a set of Simple_Selector pointers because std::set == std::set is going to call ==
    // on the pointers to determine equality. I don't know of a way to pass in a comparison object. The one you can specify as part of
    // the template type is used for ordering, but not equality. We also can't just put in non-pointer Simple_Selectors because the
    // class is intended to be subclassed, and we'd get splicing.

    set<string> lset, rset;

    for (size_t i = 0, L = length(); i < L; ++i)
    { lset.insert((*this)[i]->perform(&to_string)); }
    for (size_t i = 0, L = rhs.length(); i < L; ++i)
    { rset.insert(rhs[i]->perform(&to_string)); }

    return lset == rset;
  }

  bool Complex_Selector_Pointer_Compare::operator() (const Complex_Selector* const pLeft, const Complex_Selector* const pRight) const {
    return *pLeft < *pRight;
  }

  bool Complex_Selector::is_superselector_of(Compound_Selector* rhs, string wrapping)
  {
    return last()->head() && last()->head()->is_superselector_of(rhs, wrapping);
  }

  bool Complex_Selector::is_superselector_of(Complex_Selector* rhs, string wrapping)
  {
    Complex_Selector* lhs = this;
    To_String to_string;
    // check for selectors with leading or trailing combinators
    if (!lhs->head() || !rhs->head())
    { return false; }
    Complex_Selector* l_innermost = lhs->innermost();
    if (l_innermost->combinator() != Complex_Selector::ANCESTOR_OF)
    { return false; }
    Complex_Selector* r_innermost = rhs->innermost();
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

  size_t Complex_Selector::length()
  {
    // TODO: make this iterative
    if (!tail()) return 1;
    return 1 + tail()->length();
  }

  Complex_Selector* Complex_Selector::context(Context& ctx)
  {
    if (!tail()) return 0;
    if (!head()) return tail()->context(ctx);
    Complex_Selector* cpy = new (ctx.mem) Complex_Selector(pstate(), combinator(), head(), tail()->context(ctx));
    cpy->media_block(media_block());
    return cpy;
  }

  Selector_List* Selector_List::parentize(Selector_List* ps, Context& ctx)
  {
    Selector_List* ss = new (ctx.mem) Selector_List(pstate());
    for (size_t pi = 0, pL = ps->length(); pi < pL; ++pi) {
      for (size_t si = 0, sL = this->length(); si < sL; ++si) {
        *ss << (*this)[si]->parentize((*ps)[pi], ctx);
      }
    }
    // return selector
    return ss;
  }

  Selector_List* Selector_List::parentize(Complex_Selector* p, Context& ctx)
  {
    Selector_List* ss = new (ctx.mem) Selector_List(pstate());
    for (size_t i = 0, L = this->length(); i < L; ++i) {
      *ss << (*this)[i]->parentize(p, ctx);
    }
    // return selector
    return ss;
  }

  Complex_Selector* Complex_Selector::parentize(Context& ctx)
  {
    // create a new complex selector to return a processed copy
    return this;
    Complex_Selector* ss = new (ctx.mem) Complex_Selector(this->pstate());
    //ss->has_line_feed(this->has_line_feed());
    ss->combinator(this->combinator());
    if (this->tail()) {
      ss->tail(this->tail()->parentize(ctx));
    }
    if (Compound_Selector* head = this->head()) {
      // now add everything expect parent selectors to head
      ss->head(new (ctx.mem) Compound_Selector(head->pstate()));
      for (size_t i = 0, L = head->length(); i < L; ++i) {
        if (!dynamic_cast<Parent_Selector*>((*head)[i])) {
          *ss->head() << (*head)[i];
        }
      }
      // if (ss->head()->empty()) ss->head(0);
    }
    // return copy
    return ss;
  }

  Selector_List* Selector_List::parentize(Context& ctx)
  {
    Selector_List* ss = new (ctx.mem) Selector_List(pstate());
    for (size_t i = 0, L = length(); i < L; ++i) {
      *ss << (*this)[i]->parentize(ctx);
    }
    // return selector
    return ss;
  }

  Selector_List* Complex_Selector::parentize(Selector_List* ps, Context& ctx)
  {
    Selector_List* ss = new (ctx.mem) Selector_List(pstate());
    if (ps == 0) { *ss << this->parentize(ctx); return ss; }
    for (size_t i = 0, L = ps->length(); i < L; ++i) {
      *ss << this->parentize((*ps)[i], ctx);
    }
    // return selector
    return ss;
  }

  Complex_Selector* Complex_Selector::parentize(Complex_Selector* parent, Context& ctx)
  {
    if (!parent) return parentize(ctx);
    Complex_Selector* pr = 0;
    Compound_Selector* head = this->head();
    // create a new complex selector to return a processed copy
    Complex_Selector* ss = new (ctx.mem) Complex_Selector(pstate());
    // ss->has_line_feed(has_line_feed());
    ss->has_line_break(has_line_break());

    // Points to last complex selector
    // Moved when resolving parent refs
    Complex_Selector* cur = ss;

    // check if compound selector has exactly one parent reference
    // if so we need to connect the parent to the current selector
    // then we also need to add the remaining simple selector to the new "parent"
    if (head) {
      // create a new compound and move originals if needed
      // we may add the simple selector to the same selector
      // with parent refs we may put them in different places
      ss->head(new (ctx.mem) Compound_Selector(head->pstate()));
      ss->head()->has_parent_reference(head->has_parent_reference());
      ss->head()->has_line_break(head->has_line_break());
      // process simple selectors sequence
      for (size_t i = 0, L = head->length(); i < L; ++i) {
        // we have a parent selector in a simple selector list
        // mix parent complex selector into the compound list
        if (dynamic_cast<Parent_Selector*>((*head)[i])) {
          // clone the parent selector
          pr = parent->cloneFully(ctx);
          // assign head and tail
          cur->head(pr->head());
          cur->tail(pr->tail());
          // move forward
          cur = pr->last();
        } else {
          // just add simple selector
          *cur->head() << (*head)[i];
        }
      }
    }
    if (cur->head()) cur->head(cur->head()->length() ? cur->head() : 0);
    // parentize and assign trailing complex selector
    if (this->tail()) cur->tail(this->tail()->parentize(parent, ctx));
    // return selector
    return ss;
  }

  Complex_Selector* Complex_Selector::innermost()
  {
    if (!tail()) return this;
    else         return tail()->innermost();
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
    Complex_Selector* cpy = new (ctx.mem) Complex_Selector(*this);
    if (tail()) cpy->tail(tail()->clone(ctx));
    return cpy;
  }

  Complex_Selector* Complex_Selector::cloneFully(Context& ctx) const
  {
    Complex_Selector* cpy = new (ctx.mem) Complex_Selector(*this);

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
    Compound_Selector* cpy = new (ctx.mem) Compound_Selector(*this);
    return cpy;
  }

  Selector_List* Selector_List::clone(Context& ctx) const
  {
    Selector_List* cpy = new (ctx.mem) Selector_List(*this);
    return cpy;
  }

  Selector_List* Selector_List::cloneFully(Context& ctx) const
  {
    Selector_List* cpy = new (ctx.mem) Selector_List(pstate());
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
      if ((*this)[i]->combinator() != Complex_Selector::ANCESTOR_OF) continue;
      if ((*this)[i]->head()->is_empty_reference()) {
        Complex_Selector* tail = (*this)[i]->tail();
        // if ((*this)[i]->has_line_feed()) {
          // if (tail) tail->has_line_feed(true);
        // }
        (*this)[i] = tail;
      }
    }
  }

  void Selector_List::adjust_after_pushing(Complex_Selector* c)
  {
    if (c->has_reference())   has_reference(true);

#ifdef DEBUG
    To_String to_string;
    this->mCachedSelector(this->perform(&to_string));
#endif
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Complex_Selector::is_superselector_of(Selector_List *sub, string wrapping)
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Selector_List::is_superselector_of(Selector_List *sub, string wrapping)
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Compound_Selector *sub, string wrapping)
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub, wrapping)) return true;
    }
    return false;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Complex_Selector *sub, string wrapping)
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub)) return true;
    }
    return false;
  }

  Selector_List* Selector_List::unify_with(Selector_List* rhs, Context& ctx) {
    vector<Complex_Selector*> unified_complex_selectors;
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
    Selector_List* final_result = new (ctx.mem) Selector_List(pstate());
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
        extends.put(compound_sel->to_str_vec(), make_pair((*extender)[i], compound_sel));
      }
    }
  };

  vector<string> Compound_Selector::to_str_vec()
  {
    To_String to_string;
    vector<string> result;
    result.reserve(length());
    for (size_t i = 0, L = length(); i < L; ++i)
    { result.push_back((*this)[i]->perform(&to_string)); }
    return result;
  }

  Compound_Selector* Compound_Selector::minus(Compound_Selector* rhs, Context& ctx)
  {
    To_String to_string(&ctx);
    Compound_Selector* result = new (ctx.mem) Compound_Selector(pstate());
    // result->has_parent_reference(has_parent_reference());

    // not very efficient because it needs to preserve order
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      bool found = false;
      string thisSelector((*this)[i]->perform(&to_string));
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

  Number::Number(ParserState pstate, double val, string u, bool zero)
  : Value(pstate),
    value_(val),
    zero_(zero),
    numerator_units_(vector<string>()),
    denominator_units_(vector<string>()),
    hash_(0)
  {
    size_t l = 0, r = 0;
    if (!u.empty()) {
      bool nominator = true;
      while (true) {
        r = u.find_first_of("*/", l);
        string unit(u.substr(l, r == string::npos ? r : r - l));
        if (nominator) numerator_units_.push_back(unit);
        else denominator_units_.push_back(unit);
        if (r == string::npos) break;
        // ToDo: should error for multiple slashes
        // if (!nominator && u[r] == '/') error(...)
        if (u[r] == '/')
          nominator = false;
        l = r + 1;
      }
    }
    concrete_type(NUMBER);
  }

  string Number::unit() const
  {
    string u;
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

  void Number::normalize(const string& prefered)
  {

    // first make sure same units cancel each other out
    // it seems that a map table will fit nicely to do this
    // we basically construct exponents for each unit
    // has the advantage that they will be pre-sorted
    map<string, int> exponents;

    // initialize by summing up occurences in unit vectors
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) ++ exponents[numerator_units_[i]];
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) -- exponents[denominator_units_[i]];

    // the final conversion factor
    double factor = 1;

    // get the first entry of numerators
    // forward it when entry is converted
    vector<string>::iterator nom_it = numerator_units_.begin();
    vector<string>::iterator nom_end = numerator_units_.end();
    vector<string>::iterator denom_it = denominator_units_.begin();
    vector<string>::iterator denom_end = denominator_units_.end();

    // main normalization loop
    // should be close to optimal
    while (denom_it != denom_end)
    {
      // get and increment afterwards
      const string denom = *(denom_it ++);
      // skip already canceled out unit
      if (exponents[denom] >= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(denom) == UNKNOWN) continue;
      // now search for nominator
      while (nom_it != nom_end)
      {
        // get and increment afterwards
        const string nom = *(nom_it ++);
        // skip already canceled out unit
        if (exponents[nom] <= 0) continue;
        // skip all units we don't know how to convert
        if (string_to_unit(nom) == UNKNOWN) continue;
        // we now have two convertable units
        // add factor for current conversion
        factor *= conversion_factor(nom, denom);
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
        if (exp.second < 0) denominator_units_.push_back(exp.first);
        else if (exp.second > 0) numerator_units_.push_back(exp.first);
      }
    }

    // apply factor to value_
    // best precision this way
    value_ *= factor;

    // maybe convert to other unit
    // easier implemented on its own
    try { convert(prefered); }
    catch (incompatibleUnits& err)
    { error(err.what(), pstate()); }
    catch (...) { throw; }

  }

  void Number::convert(const string& prefered)
  {
    // abort if unit is empty
    if (prefered.empty()) return;

    // first make sure same units cancel each other out
    // it seems that a map table will fit nicely to do this
    // we basically construct exponents for each unit
    // has the advantage that they will be pre-sorted
    map<string, int> exponents;

    // initialize by summing up occurences in unit vectors
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) ++ exponents[numerator_units_[i]];
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) -- exponents[denominator_units_[i]];

    // the final conversion factor
    double factor = 1;

    vector<string>::iterator denom_it = denominator_units_.begin();
    vector<string>::iterator denom_end = denominator_units_.end();

    // main normalization loop
    // should be close to optimal
    while (denom_it != denom_end)
    {
      // get and increment afterwards
      const string denom = *(denom_it ++);
      // check if conversion is needed
      if (denom == prefered) continue;
      // skip already canceled out unit
      if (exponents[denom] >= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(denom) == UNKNOWN) continue;
      // we now have two convertable units
      // add factor for current conversion
      factor *= conversion_factor(denom, prefered);
      // update nominator/denominator exponent
      ++ exponents[denom]; -- exponents[prefered];
    }

    vector<string>::iterator nom_it = numerator_units_.begin();
    vector<string>::iterator nom_end = numerator_units_.end();

    // now search for nominator
    while (nom_it != nom_end)
    {
      // get and increment afterwards
      const string nom = *(nom_it ++);
      // check if conversion is needed
      if (nom == prefered) continue;
      // skip already canceled out unit
      if (exponents[nom] <= 0) continue;
      // skip all units we don't know how to convert
      if (string_to_unit(nom) == UNKNOWN) continue;
      // we now have two convertable units
      // add factor for current conversion
      factor *= conversion_factor(nom, prefered);
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
        if (exp.second < 0) denominator_units_.push_back(exp.first);
        else if (exp.second > 0) numerator_units_.push_back(exp.first);
      }
    }

    // apply factor to value_
    // best precision this way
    value_ *= factor;

  }

  // useful for making one number compatible with another
  string Number::find_convertible_unit() const
  {
    for (size_t i = 0, S = numerator_units_.size(); i < S; ++i) {
      string u(numerator_units_[i]);
      if (string_to_unit(u) != UNKNOWN) return u;
    }
    for (size_t i = 0, S = denominator_units_.size(); i < S; ++i) {
      string u(denominator_units_[i]);
      if (string_to_unit(u) != UNKNOWN) return u;
    }
    return string();
  }

  bool Custom_Warning::operator== (Expression* rhs) const
  {
    if (Custom_Warning* r = dynamic_cast<Custom_Warning*>(rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Custom_Warning::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Custom_Error::operator== (Expression* rhs) const
  {
    if (Custom_Error* r = dynamic_cast<Custom_Error*>(rhs)) {
      return message() == r->message();
    }
    return false;
  }

  bool Custom_Error::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Number::operator== (Expression* rhs) const
  {
    if (Number* r = dynamic_cast<Number*>(rhs)) {
      return (value() == r->value()) &&
             (numerator_units_ == r->numerator_units_) &&
             (denominator_units_ == r->denominator_units_);
    }
    return false;
  }

  bool Number::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Number::operator< (Number* rhs) const
  {
    Number tmp_r(*rhs);
    tmp_r.normalize(find_convertible_unit());
    string l_unit(unit());
    string r_unit(tmp_r.unit());
    if (!l_unit.empty() && !r_unit.empty() && unit() != tmp_r.unit()) {
      error("cannot compare numbers with incompatible units", pstate());
    }
    return value() < tmp_r.value();
  }

  bool Number::operator< (Number& rhs) const
  {
    return operator<(&rhs);
  }

  bool String_Quoted::operator== (Expression* rhs) const
  {
    if (String_Quoted* qstr = dynamic_cast<String_Quoted*>(rhs)) {
      return (value() == qstr->value());
    } else if (String_Constant* cstr = dynamic_cast<String_Constant*>(rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Quoted::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool String_Constant::operator== (Expression* rhs) const
  {
    if (String_Quoted* qstr = dynamic_cast<String_Quoted*>(rhs)) {
      return (value() == qstr->value());
    } else if (String_Constant* cstr = dynamic_cast<String_Constant*>(rhs)) {
      return (value() == cstr->value());
    }
    return false;
  }

  bool String_Constant::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool String_Schema::operator== (Expression* rhs) const
  {
    if (String_Schema* r = dynamic_cast<String_Schema*>(rhs)) {
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

  bool String_Schema::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Boolean::operator== (Expression* rhs) const
  {
    if (Boolean* r = dynamic_cast<Boolean*>(rhs)) {
      return (value() == r->value());
    }
    return false;
  }

  bool Boolean::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Color::operator== (Expression* rhs) const
  {
    if (Color* r = dynamic_cast<Color*>(rhs)) {
      return r_ == r->r() &&
             g_ == r->g() &&
             b_ == r->b() &&
             a_ == r->a();
    }
    return false;
  }

  bool Color::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool List::operator== (Expression* rhs) const
  {
    if (List* r = dynamic_cast<List*>(rhs)) {
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

  bool List::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Map::operator== (Expression* rhs) const
  {
    if (Map* r = dynamic_cast<Map*>(rhs)) {
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

  bool Map::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
  }

  bool Null::operator== (Expression* rhs) const
  {
    return rhs->concrete_type() == NULL_VAL;
  }

  bool Null::operator== (Expression& rhs) const
  {
    return operator==(&rhs);
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

  string Map::to_string(bool compressed, int precision) const
  {
    string res("");
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

  string List::to_string(bool compressed, int precision) const
  {
    string res("");
    if (empty()) return res;
    if (is_invisible()) return res;
    bool items_output = false;
    string sep = separator() == SASS_COMMA ? "," : " ";
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

  string String_Schema::to_string(bool compressed, int precision) const
  {
    string res("");
    for (size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_interpolant()) res += "#{";
      if (Value* val = dynamic_cast<Value*>((*this)[i]))
      { res += val->to_string(compressed, precision); }
      if ((*this)[i]->is_interpolant()) res += "}";
    }
    return res;
  }

  string Null::to_string(bool compressed, int precision) const
  {
    return "null";
  }

  string Boolean::to_string(bool compressed, int precision) const
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

  string Color::to_string(bool compressed, int precision) const
  {
    stringstream ss;

    // original color name
    // maybe an unknown token
    string name = disp();

    // resolved color
    string res_name = name;

    double r = round(cap_channel<0xff>(r_));
    double g = round(cap_channel<0xff>(g_));
    double b = round(cap_channel<0xff>(b_));
    double a = cap_channel<1>   (a_);

    // get color from given name (if one was given at all)
    if (name != "" && names_to_colors.count(name)) {
      Color* n = names_to_colors.find(name)->second;
      r = round(cap_channel<0xff>(n->r()));
      g = round(cap_channel<0xff>(n->g()));
      b = round(cap_channel<0xff>(n->b()));
      a = cap_channel<1>   (n->a());
    }
    // otherwise get the possible resolved color name
    else {
      int numval = static_cast<int>(r) * 0x10000 + static_cast<int>(g) * 0x100 + static_cast<int>(b);
      if (colors_to_names.count(numval))
        res_name = colors_to_names.find(numval)->second;
    }

    stringstream hexlet;
    hexlet << '#' << setw(1) << setfill('0');
    // create a short color hexlet if there is any need for it
    if (compressed && is_color_doublet(r, g, b) && a == 1) {
      hexlet << hex << setw(1) << (static_cast<unsigned long>(r) >> 4);
      hexlet << hex << setw(1) << (static_cast<unsigned long>(g) >> 4);
      hexlet << hex << setw(1) << (static_cast<unsigned long>(b) >> 4);
    } else {
      hexlet << hex << setw(2) << static_cast<unsigned long>(r);
      hexlet << hex << setw(2) << static_cast<unsigned long>(g);
      hexlet << hex << setw(2) << static_cast<unsigned long>(b);
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

  string Number::to_string(bool compressed, int precision) const
  {

    string res;

    // check if the fractional part of the value equals to zero
    // neat trick from http://stackoverflow.com/a/1521682/1550314
    // double int_part; bool is_int = modf(value, &int_part) == 0.0;

    // this all cannot be done with one run only, since fixed
    // output differs from normal output and regular output
    // can contain scientific notation which we do not want!

    // first sample
    stringstream ss;
    ss.precision(12);
    ss << value_;

    // check if we got scientific notation in result
    if (ss.str().find_first_of("e") != string::npos) {
      ss.clear(); ss.str(string());
      ss.precision(max(12, precision));
      ss << fixed << value_;
    }

    string tmp = ss.str();
    size_t pos_point = tmp.find_first_of(".,");
    size_t pos_fract = tmp.find_last_not_of("0");
    bool is_int = pos_point == pos_fract ||
                  pos_point == string::npos;

    // reset stream for another run
    ss.clear(); ss.str(string());

    // take a shortcut for integers
    if (is_int)
    {
      ss.precision(0);
      ss << fixed << value_;
      res = string(ss.str());
    }
    // process floats
    else
    {
      // do we have have too much precision?
      if (pos_fract < precision + pos_point)
      { precision = pos_fract - pos_point; }
      // round value again
      ss.precision(precision);
      ss << fixed << value_;
      res = string(ss.str());
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

    // add unit now
    res += unit();

    // and return
    return res;

  }

  string String_Quoted::to_string(bool compressed, int precision) const
  {
    return quote_mark_ ? quote(value_, quote_mark_, true) : value_;
  }

  string String_Constant::to_string(bool compressed, int precision) const
  {
    return quote_mark_ ? quote(value_, quote_mark_, true) : value_;
  }

  string Custom_Error::to_string(bool compressed, int precision) const
  {
    return message();
  }
  string Custom_Warning::to_string(bool compressed, int precision) const
  {
    return message();
  }

}

