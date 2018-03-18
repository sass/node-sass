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

namespace Sass {

  bool Wrapped_Selector::find ( bool (*f)(AST_Node_Obj) )
  {
    // check children first
    if (selector_) {
      if (selector_->find(f)) return true;
    }
    // execute last
    return f(this);
  }

  bool Selector_List::find ( bool (*f)(AST_Node_Obj) )
  {
    // check children first
    for (Complex_Selector_Obj sel : elements()) {
      if (sel->find(f)) return true;
    }
    // execute last
    return f(this);
  }

  bool Compound_Selector::find ( bool (*f)(AST_Node_Obj) )
  {
    // check children first
    for (Simple_Selector_Obj sel : elements()) {
      if (sel->find(f)) return true;
    }
    // execute last
    return f(this);
  }

  bool Complex_Selector::find ( bool (*f)(AST_Node_Obj) )
  {
    // check children first
    if (head_ && head_->find(f)) return true;
    if (tail_ && tail_->find(f)) return true;
    // execute last
    return f(this);
  }

  bool Simple_Selector::is_ns_eq(const Simple_Selector& r) const
  {
    // https://github.com/sass/sass/issues/2229
    if ((has_ns_ == r.has_ns_) ||
        (has_ns_ && ns_.empty()) ||
        (r.has_ns_ && r.ns_.empty())
    ) {
      if (ns_.empty() && r.ns() == "*") return false;
      else if (r.ns().empty() && ns() == "*") return false;
      else return ns() == r.ns();
    }
    return false;
  }

  bool Compound_Selector::has_parent_ref() const
  {
    for (Simple_Selector_Obj s : *this) {
      if (s && s->has_parent_ref()) return true;
    }
    return false;
  }

  bool Compound_Selector::has_real_parent_ref() const
  {
    for (Simple_Selector_Obj s : *this) {
      if (s && s->has_real_parent_ref()) return true;
    }
    return false;
  }

  bool Complex_Selector::has_parent_ref() const
  {
    return (head() && head()->has_parent_ref()) ||
           (tail() && tail()->has_parent_ref());
  }

  bool Complex_Selector::has_real_parent_ref() const
  {
    return (head() && head()->has_real_parent_ref()) ||
           (tail() && tail()->has_real_parent_ref());
  }

  bool Wrapped_Selector::is_superselector_of(Wrapped_Selector_Ptr_Const sub) const
  {
    if (this->name() != sub->name()) return false;
    if (this->name() == ":current") return false;
    if (Selector_List_Obj rhs_list = Cast<Selector_List>(sub->selector())) {
      if (Selector_List_Obj lhs_list = Cast<Selector_List>(selector())) {
        return lhs_list->is_superselector_of(rhs_list);
      }
    }
    coreError("is_superselector expected a Selector_List", sub->pstate());
    return false;
  }

  bool Compound_Selector::is_superselector_of(Selector_List_Ptr_Const rhs, std::string wrapped) const
  {
    for (Complex_Selector_Obj item : rhs->elements()) {
      if (is_superselector_of(item, wrapped)) return true;
    }
    return false;
  }

  bool Compound_Selector::is_superselector_of(Complex_Selector_Ptr_Const rhs, std::string wrapped) const
  {
    if (rhs->head()) return is_superselector_of(rhs->head(), wrapped);
    return false;
  }

  bool Compound_Selector::is_superselector_of(Compound_Selector_Ptr_Const rhs, std::string wrapping) const
  {
    Compound_Selector_Ptr_Const lhs = this;
    Simple_Selector_Ptr lbase = lhs->base();
    Simple_Selector_Ptr rbase = rhs->base();

    // Check if pseudo-elements are the same between the selectors

    std::set<std::string> lpsuedoset, rpsuedoset;
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      if ((*this)[i]->is_pseudo_element()) {
        std::string pseudo((*this)[i]->to_string());
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        lpsuedoset.insert(pseudo);
      }
    }
    for (size_t i = 0, L = rhs->length(); i < L; ++i)
    {
      if ((*rhs)[i]->is_pseudo_element()) {
        std::string pseudo((*rhs)[i]->to_string());
        pseudo = pseudo.substr(pseudo.find_first_not_of(":")); // strip off colons to ensure :after matches ::after since ruby sass is forgiving
        rpsuedoset.insert(pseudo);
      }
    }
    if (lpsuedoset != rpsuedoset) {
      return false;
    }

    // would like to replace this without stringification
    // https://github.com/sass/sass/issues/2229
    // SimpleSelectorSet lset, rset;
    std::set<std::string> lset, rset;

    if (lbase && rbase)
    {
      if (lbase->to_string() == rbase->to_string()) {
        for (size_t i = 1, L = length(); i < L; ++i)
        { lset.insert((*this)[i]->to_string()); }
        for (size_t i = 1, L = rhs->length(); i < L; ++i)
        { rset.insert((*rhs)[i]->to_string()); }
        return includes(rset.begin(), rset.end(), lset.begin(), lset.end());
      }
      return false;
    }

    for (size_t i = 0, iL = length(); i < iL; ++i)
    {
      Selector_Obj wlhs = (*this)[i];
      // very special case for wrapped matches selector
      if (Wrapped_Selector_Obj wrapped = Cast<Wrapped_Selector>(wlhs)) {
        if (wrapped->name() == ":not") {
          if (Selector_List_Obj not_list = Cast<Selector_List>(wrapped->selector())) {
            if (not_list->is_superselector_of(rhs, wrapped->name())) return false;
          } else {
            throw std::runtime_error("wrapped not selector is not a list");
          }
        }
        if (wrapped->name() == ":matches" || wrapped->name() == ":-moz-any") {
          wlhs = wrapped->selector();
          if (Selector_List_Obj list = Cast<Selector_List>(wrapped->selector())) {
            if (Compound_Selector_Ptr_Const comp = Cast<Compound_Selector>(rhs)) {
              if (!wrapping.empty() && wrapping != wrapped->name()) return false;
              if (wrapping.empty() || wrapping != wrapped->name()) {;
                if (list->is_superselector_of(comp, wrapped->name())) return true;
              }
            }
          }
        }
        Simple_Selector_Ptr rhs_sel = NULL;
        if (rhs->elements().size() > i) rhs_sel = (*rhs)[i];
        if (Wrapped_Selector_Ptr wrapped_r = Cast<Wrapped_Selector>(rhs_sel)) {
          if (wrapped->name() == wrapped_r->name()) {
          if (wrapped->is_superselector_of(wrapped_r)) {
             continue;
          }}
        }
      }
      // match from here on as strings
      lset.insert(wlhs->to_string());
    }

    for (size_t n = 0, nL = rhs->length(); n < nL; ++n)
    {
      Selector_Obj r = (*rhs)[n];
      if (Wrapped_Selector_Obj wrapped = Cast<Wrapped_Selector>(r)) {
        if (wrapped->name() == ":not") {
          if (Selector_List_Obj ls = Cast<Selector_List>(wrapped->selector())) {
            ls->remove_parent_selectors();
            if (is_superselector_of(ls, wrapped->name())) return false;
          }
        }
        if (wrapped->name() == ":matches" || wrapped->name() == ":-moz-any") {
          if (!wrapping.empty()) {
            if (wrapping != wrapped->name()) return false;
          }
          if (Selector_List_Obj ls = Cast<Selector_List>(wrapped->selector())) {
            ls->remove_parent_selectors();
            return (is_superselector_of(ls, wrapped->name()));
          }
        }
      }
      rset.insert(r->to_string());
    }

    //for (auto l : lset) { cerr << "l: " << l << endl; }
    //for (auto r : rset) { cerr << "r: " << r << endl; }

    if (lset.empty()) return true;
    // return true if rset contains all the elements of lset
    return includes(rset.begin(), rset.end(), lset.begin(), lset.end());

  }

  // create complex selector (ancestor of) from compound selector
  Complex_Selector_Obj Compound_Selector::to_complex()
  {
    // create an intermediate complex selector
    return SASS_MEMORY_NEW(Complex_Selector,
                           pstate(),
                           Complex_Selector::ANCESTOR_OF,
                           this,
                           {});
  }

  bool Complex_Selector::is_superselector_of(Compound_Selector_Ptr_Const rhs, std::string wrapping) const
  {
    return last()->head() && last()->head()->is_superselector_of(rhs, wrapping);
  }

  bool Complex_Selector::is_superselector_of(Complex_Selector_Ptr_Const rhs, std::string wrapping) const
  {
    Complex_Selector_Ptr_Const lhs = this;
    // check for selectors with leading or trailing combinators
    if (!lhs->head() || !rhs->head())
    { return false; }
    Complex_Selector_Ptr_Const l_innermost = lhs->last();
    if (l_innermost->combinator() != Complex_Selector::ANCESTOR_OF)
    { return false; }
    Complex_Selector_Ptr_Const r_innermost = rhs->last();
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
      Complex_Selector_Obj lhs_tail = lhs->tail();
      Complex_Selector_Obj rhs_tail = rhs->tail();
      if (lhs_tail->combinator() != rhs_tail->combinator()) return false;
      if (lhs_tail->head() && !rhs_tail->head()) return false;
      if (!lhs_tail->head() && rhs_tail->head()) return false;
      if (lhs_tail->head() && rhs_tail->head()) {
        if (!lhs_tail->head()->is_superselector_of(rhs_tail->head())) return false;
      }
    }

    bool found = false;
    Complex_Selector_Ptr_Const marker = rhs;
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
    return lhs->tail()->is_superselector_of(marker->tail());
  }

  size_t Complex_Selector::length() const
  {
    // TODO: make this iterative
    if (!tail()) return 1;
    return 1 + tail()->length();
  }

  // append another complex selector at the end
  // check if we need to append some headers
  // then we need to check for the combinator
  // only then we can safely set the new tail
  void Complex_Selector::append(Complex_Selector_Obj ss, Backtraces& traces)
  {

    Complex_Selector_Obj t = ss->tail();
    Combinator c = ss->combinator();
    String_Obj r = ss->reference();
    Compound_Selector_Obj h = ss->head();

    if (ss->has_line_feed()) has_line_feed(true);
    if (ss->has_line_break()) has_line_break(true);

    // append old headers
    if (h && h->length()) {
      if (last()->combinator() != ANCESTOR_OF && c != ANCESTOR_OF) {
        traces.push_back(Backtrace(pstate()));
        throw Exception::InvalidParent(this, traces, ss);
      } else if (last()->head_ && last()->head_->length()) {
        Compound_Selector_Obj rh = last()->head();
        size_t i;
        size_t L = h->length();
        if (Cast<Type_Selector>(h->first())) {
          if (Class_Selector_Ptr cs = Cast<Class_Selector>(rh->last())) {
            Class_Selector_Ptr sqs = SASS_MEMORY_COPY(cs);
            sqs->name(sqs->name() + (*h)[0]->name());
            sqs->pstate((*h)[0]->pstate());
            (*rh)[rh->length()-1] = sqs;
            rh->pstate(h->pstate());
            for (i = 1; i < L; ++i) rh->append((*h)[i]);
          } else if (Id_Selector_Ptr is = Cast<Id_Selector>(rh->last())) {
            Id_Selector_Ptr sqs = SASS_MEMORY_COPY(is);
            sqs->name(sqs->name() + (*h)[0]->name());
            sqs->pstate((*h)[0]->pstate());
            (*rh)[rh->length()-1] = sqs;
            rh->pstate(h->pstate());
            for (i = 1; i < L; ++i) rh->append((*h)[i]);
          } else if (Type_Selector_Ptr ts = Cast<Type_Selector>(rh->last())) {
            Type_Selector_Ptr tss = SASS_MEMORY_COPY(ts);
            tss->name(tss->name() + (*h)[0]->name());
            tss->pstate((*h)[0]->pstate());
            (*rh)[rh->length()-1] = tss;
            rh->pstate(h->pstate());
            for (i = 1; i < L; ++i) rh->append((*h)[i]);
          } else if (Placeholder_Selector_Ptr ps = Cast<Placeholder_Selector>(rh->last())) {
            Placeholder_Selector_Ptr pss = SASS_MEMORY_COPY(ps);
            pss->name(pss->name() + (*h)[0]->name());
            pss->pstate((*h)[0]->pstate());
            (*rh)[rh->length()-1] = pss;
            rh->pstate(h->pstate());
            for (i = 1; i < L; ++i) rh->append((*h)[i]);
          } else {
            last()->head_->concat(h);
          }
        } else {
          last()->head_->concat(h);
        }
      } else if (last()->head_) {
        last()->head_->concat(h);
      }
    } else {
      // std::cerr << "has no or empty head\n";
    }

    Complex_Selector_Ptr last = mutable_last();
    if (last) {
      if (last->combinator() != ANCESTOR_OF && c != ANCESTOR_OF) {
        Complex_Selector_Ptr inter = SASS_MEMORY_NEW(Complex_Selector, pstate());
        inter->reference(r);
        inter->combinator(c);
        inter->tail(t);
        last->tail(inter);
      } else {
        if (last->combinator() == ANCESTOR_OF) {
          last->combinator(c);
          last->reference(r);
        }
        last->tail(t);
      }
    }

  }

  Selector_List_Obj Selector_List::eval(Eval& eval)
  {
    Selector_List_Obj list = schema() ?
      eval(schema()) : eval(this);
    list->schema(schema());
    return list;
  }

  Selector_List_Ptr Selector_List::resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent)
  {
    if (!this->has_parent_ref()) return this;
    Selector_List_Ptr ss = SASS_MEMORY_NEW(Selector_List, pstate());
    for (size_t si = 0, sL = this->length(); si < sL; ++si) {
      Selector_List_Obj rv = at(si)->resolve_parent_refs(pstack, traces, implicit_parent);
      ss->concat(rv);
    }
    return ss;
  }

  Selector_List_Ptr Complex_Selector::resolve_parent_refs(SelectorStack& pstack, Backtraces& traces, bool implicit_parent)
  {
    Complex_Selector_Obj tail = this->tail();
    Compound_Selector_Obj head = this->head();
    Selector_List_Ptr parents = pstack.back();

    if (!this->has_real_parent_ref() && !implicit_parent) {
      Selector_List_Ptr retval = SASS_MEMORY_NEW(Selector_List, pstate(), 1);
      retval->append(this);
      return retval;
    }

    // first resolve_parent_refs the tail (which may return an expanded list)
    Selector_List_Obj tails = tail ? tail->resolve_parent_refs(pstack, traces, implicit_parent) : 0;

    if (head && head->length() > 0) {

      Selector_List_Obj retval;
      // we have a parent selector in a simple compound list
      // mix parent complex selector into the compound list
      if (Cast<Parent_Selector>((*head)[0])) {
        retval = SASS_MEMORY_NEW(Selector_List, pstate());

        // it turns out that real parent references reach
        // across @at-root rules, which comes unexpected
        if (parents == NULL && head->has_real_parent_ref()) {
          int i = pstack.size() - 1;
          while (!parents && i > -1) {
            parents = pstack.at(i--);
          }
        }

        if (parents && parents->length()) {
          if (tails && tails->length() > 0) {
            for (size_t n = 0, nL = tails->length(); n < nL; ++n) {
              for (size_t i = 0, iL = parents->length(); i < iL; ++i) {
                Complex_Selector_Obj t = (*tails)[n];
                Complex_Selector_Obj parent = (*parents)[i];
                Complex_Selector_Obj s = SASS_MEMORY_CLONE(parent);
                Complex_Selector_Obj ss = SASS_MEMORY_CLONE(this);
                ss->tail(t ? SASS_MEMORY_CLONE(t) : NULL);
                Compound_Selector_Obj h = SASS_MEMORY_COPY(head_);
                // remove parent selector from sequence
                if (h->length()) {
                  h->erase(h->begin());
                  ss->head(h);
                } else {
                  ss->head({});
                }
                // adjust for parent selector (1 char)
                // if (h->length()) {
                //   ParserState state(h->at(0)->pstate());
                //   state.offset.column += 1;
                //   state.column -= 1;
                //   (*h)[0]->pstate(state);
                // }
                // keep old parser state
                s->pstate(pstate());
                // append new tail
                s->append(ss, traces);
                retval->append(s);
              }
            }
          }
          // have no tails but parents
          // loop above is inside out
          else {
            for (size_t i = 0, iL = parents->length(); i < iL; ++i) {
              Complex_Selector_Obj parent = (*parents)[i];
              Complex_Selector_Obj s = SASS_MEMORY_CLONE(parent);
              Complex_Selector_Obj ss = SASS_MEMORY_CLONE(this);
              // this is only if valid if the parent has no trailing op
              // otherwise we cannot append more simple selectors to head
              if (parent->last()->combinator() != ANCESTOR_OF) {
                traces.push_back(Backtrace(pstate()));
                throw Exception::InvalidParent(parent, traces, ss);
              }
              ss->tail(tail ? SASS_MEMORY_CLONE(tail) : NULL);
              Compound_Selector_Obj h = SASS_MEMORY_COPY(head_);
              // remove parent selector from sequence
              if (h->length()) {
                h->erase(h->begin());
                ss->head(h);
              } else {
                ss->head({});
              }
              // \/ IMO ruby sass bug \/
              ss->has_line_feed(false);
              // adjust for parent selector (1 char)
              // if (h->length()) {
              //   ParserState state(h->at(0)->pstate());
              //   state.offset.column += 1;
              //   state.column -= 1;
              //   (*h)[0]->pstate(state);
              // }
              // keep old parser state
              s->pstate(pstate());
              // append new tail
              s->append(ss, traces);
              retval->append(s);
            }
          }
        }
        // have no parent but some tails
        else {
          if (tails && tails->length() > 0) {
            for (size_t n = 0, nL = tails->length(); n < nL; ++n) {
              Complex_Selector_Obj cpy = SASS_MEMORY_CLONE(this);
              cpy->tail(SASS_MEMORY_CLONE(tails->at(n)));
              cpy->head(SASS_MEMORY_NEW(Compound_Selector, head->pstate()));
              for (size_t i = 1, L = this->head()->length(); i < L; ++i)
                cpy->head()->append((*this->head())[i]);
              if (!cpy->head()->length()) cpy->head({});
              retval->append(cpy->skip_empty_reference());
            }
          }
          // have no parent nor tails
          else {
            Complex_Selector_Obj cpy = SASS_MEMORY_CLONE(this);
            cpy->head(SASS_MEMORY_NEW(Compound_Selector, head->pstate()));
            for (size_t i = 1, L = this->head()->length(); i < L; ++i)
              cpy->head()->append((*this->head())[i]);
            if (!cpy->head()->length()) cpy->head({});
            retval->append(cpy->skip_empty_reference());
          }
        }
      }
      // no parent selector in head
      else {
        retval = this->tails(tails);
      }

      for (Simple_Selector_Obj ss : head->elements()) {
        if (Wrapped_Selector_Ptr ws = Cast<Wrapped_Selector>(ss)) {
          if (Selector_List_Ptr sl = Cast<Selector_List>(ws->selector())) {
            if (parents) ws->selector(sl->resolve_parent_refs(pstack, traces, implicit_parent));
          }
        }
      }

      return retval.detach();

    }
    // has no head
    return this->tails(tails);
  }

  Selector_List_Ptr Complex_Selector::tails(Selector_List_Ptr tails)
  {
    Selector_List_Ptr rv = SASS_MEMORY_NEW(Selector_List, pstate_);
    if (tails && tails->length()) {
      for (size_t i = 0, iL = tails->length(); i < iL; ++i) {
        Complex_Selector_Obj pr = SASS_MEMORY_CLONE(this);
        pr->tail(tails->at(i));
        rv->append(pr);
      }
    }
    else {
      rv->append(this);
    }
    return rv;
  }

  // return the last tail that is defined
  Complex_Selector_Ptr_Const Complex_Selector::first() const
  {
    // declare variables used in loop
    Complex_Selector_Ptr_Const cur = this;
    Compound_Selector_Ptr_Const head;
    // processing loop
    while (cur)
    {
      // get the head
      head = cur->head_.ptr();
      // abort (and return) if it is not a parent selector
      if (!head || head->length() != 1 || !Cast<Parent_Selector>((*head)[0])) {
        break;
      }
      // advance to next
      cur = cur->tail_;
    }
    // result
    return cur;
  }

  Complex_Selector_Ptr Complex_Selector::mutable_first()
  {
    return const_cast<Complex_Selector_Ptr>(first());
  }

  // return the last tail that is defined
  Complex_Selector_Ptr_Const Complex_Selector::last() const
  {
    Complex_Selector_Ptr_Const cur = this;
    Complex_Selector_Ptr_Const nxt = cur;
    // loop until last
    while (nxt) {
      cur = nxt;
      nxt = cur->tail_.ptr();
    }
    return cur;
  }

  Complex_Selector_Ptr Complex_Selector::mutable_last()
  {
    return const_cast<Complex_Selector_Ptr>(last());
  }

  Complex_Selector::Combinator Complex_Selector::clear_innermost()
  {
    Combinator c;
    if (!tail() || tail()->tail() == nullptr)
    { c = combinator(); combinator(ANCESTOR_OF); tail({}); }
    else
    { c = tail_->clear_innermost(); }
    return c;
  }

  void Complex_Selector::set_innermost(Complex_Selector_Obj val, Combinator c)
  {
    if (!tail_)
    { tail_ = val; combinator(c); }
    else
    { tail_->set_innermost(val, c); }
  }

  void Complex_Selector::cloneChildren()
  {
    if (head()) head(SASS_MEMORY_CLONE(head()));
    if (tail()) tail(SASS_MEMORY_CLONE(tail()));
  }

  void Compound_Selector::cloneChildren()
  {
    for (size_t i = 0, l = length(); i < l; i++) {
      at(i) = SASS_MEMORY_CLONE(at(i));
    }
  }

  void Selector_List::cloneChildren()
  {
    for (size_t i = 0, l = length(); i < l; i++) {
      at(i) = SASS_MEMORY_CLONE(at(i));
    }
  }

  void Wrapped_Selector::cloneChildren()
  {
    selector(SASS_MEMORY_CLONE(selector()));
  }

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
          if ((*this)[i]->tail()) {
            if ((*this)[i]->has_line_feed()) {
              (*this)[i]->tail()->has_line_feed(true);
            }
            (*this)[i] = (*this)[i]->tail();
          }
        }
        // otherwise remove the first item from head
        else {
          (*this)[i]->head()->erase((*this)[i]->head()->begin());
        }
      }
    }
  }

  size_t Wrapped_Selector::hash() const
  {
    if (hash_ == 0) {
      hash_combine(hash_, Simple_Selector::hash());
      if (selector_) hash_combine(hash_, selector_->hash());
    }
    return hash_;
  }
  bool Wrapped_Selector::has_parent_ref() const {
    // if (has_reference()) return true;
    if (!selector()) return false;
    return selector()->has_parent_ref();
  }
  bool Wrapped_Selector::has_real_parent_ref() const {
    // if (has_reference()) return true;
    if (!selector()) return false;
    return selector()->has_real_parent_ref();
  }
  unsigned long Wrapped_Selector::specificity() const
  {
    return selector_ ? selector_->specificity() : 0;
  }


  bool Selector_List::has_parent_ref() const
  {
    for (Complex_Selector_Obj s : elements()) {
      if (s && s->has_parent_ref()) return true;
    }
    return false;
  }

  bool Selector_List::has_real_parent_ref() const
  {
    for (Complex_Selector_Obj s : elements()) {
      if (s && s->has_real_parent_ref()) return true;
    }
    return false;
  }

  bool Selector_Schema::has_parent_ref() const
  {
    if (String_Schema_Obj schema = Cast<String_Schema>(contents())) {
      return schema->length() > 0 && Cast<Parent_Selector>(schema->at(0)) != NULL;
    }
    return false;
  }

  bool Selector_Schema::has_real_parent_ref() const
  {
    if (String_Schema_Obj schema = Cast<String_Schema>(contents())) {
      if (schema->length() == 0) return false;
      return Cast<Parent_Reference>(schema->at(0)) != nullptr;
    }
    return false;
  }

  void Selector_List::adjust_after_pushing(Complex_Selector_Obj c)
  {
    // if (c->has_reference())   has_reference(true);
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Complex_Selector::is_superselector_of(Selector_List_Ptr_Const sub, std::string wrapping) const
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector of the right side
  // list is a superselector of the given left side selector
  bool Selector_List::is_superselector_of(Selector_List_Ptr_Const sub, std::string wrapping) const
  {
    // Check every rhs selector against left hand list
    for(size_t i = 0, L = sub->length(); i < L; ++i) {
      if (!is_superselector_of((*sub)[i], wrapping)) return false;
    }
    return true;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Compound_Selector_Ptr_Const sub, std::string wrapping) const
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub, wrapping)) return true;
    }
    return false;
  }

  // it's a superselector if every selector on the right side
  // is a superselector of any one of the left side selectors
  bool Selector_List::is_superselector_of(Complex_Selector_Ptr_Const sub, std::string wrapping) const
  {
    // Check every lhs selector against right hand
    for(size_t i = 0, L = length(); i < L; ++i) {
      if ((*this)[i]->is_superselector_of(sub)) return true;
    }
    return false;
  }

  void Selector_List::populate_extends(Selector_List_Obj extendee, Subset_Map& extends)
  {

    Selector_List_Ptr extender = this;
    for (auto complex_sel : extendee->elements()) {
      Complex_Selector_Obj c = complex_sel;


      // Ignore any parent selectors, until we find the first non Selectorerence head
      Compound_Selector_Obj compound_sel = c->head();
      Complex_Selector_Obj pIter = complex_sel;
      while (pIter) {
        Compound_Selector_Obj pHead = pIter->head();
        if (pHead && Cast<Parent_Selector>(pHead->elements()[0]) == NULL) {
          compound_sel = pHead;
          break;
        }

        pIter = pIter->tail();
      }

      if (!pIter->head() || pIter->tail()) {
        coreError("nested selectors may not be extended", c->pstate());
      }

      compound_sel->is_optional(extendee->is_optional());

      for (size_t i = 0, L = extender->length(); i < L; ++i) {
        extends.put(compound_sel, std::make_pair((*extender)[i], compound_sel));
      }
    }
  };

  void Compound_Selector::append(Simple_Selector_Obj element)
  {
    Vectorized<Simple_Selector_Obj>::append(element);
    pstate_.offset += element->pstate().offset;
  }

  Compound_Selector_Ptr Compound_Selector::minus(Compound_Selector_Ptr rhs)
  {
    Compound_Selector_Ptr result = SASS_MEMORY_NEW(Compound_Selector, pstate());
    // result->has_parent_reference(has_parent_reference());

    // not very efficient because it needs to preserve order
    for (size_t i = 0, L = length(); i < L; ++i)
    {
      bool found = false;
      std::string thisSelector((*this)[i]->to_string());
      for (size_t j = 0, M = rhs->length(); j < M; ++j)
      {
        if (thisSelector == (*rhs)[j]->to_string())
        {
          found = true;
          break;
        }
      }
      if (!found) result->append((*this)[i]);
    }

    return result;
  }

  void Compound_Selector::mergeSources(ComplexSelectorSet& sources)
  {
    for (ComplexSelectorSet::iterator iterator = sources.begin(), endIterator = sources.end(); iterator != endIterator; ++iterator) {
      this->sources_.insert(SASS_MEMORY_CLONE(*iterator));
    }
  }

  IMPLEMENT_AST_OPERATORS(Selector_Schema);
  IMPLEMENT_AST_OPERATORS(Placeholder_Selector);
  IMPLEMENT_AST_OPERATORS(Parent_Selector);
  IMPLEMENT_AST_OPERATORS(Attribute_Selector);
  IMPLEMENT_AST_OPERATORS(Compound_Selector);
  IMPLEMENT_AST_OPERATORS(Complex_Selector);
  IMPLEMENT_AST_OPERATORS(Type_Selector);
  IMPLEMENT_AST_OPERATORS(Class_Selector);
  IMPLEMENT_AST_OPERATORS(Id_Selector);
  IMPLEMENT_AST_OPERATORS(Pseudo_Selector);
  IMPLEMENT_AST_OPERATORS(Wrapped_Selector);
  IMPLEMENT_AST_OPERATORS(Selector_List);

}