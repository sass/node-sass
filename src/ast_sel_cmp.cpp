// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
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

  /*#########################################################################*/
  /*#########################################################################*/

  bool Selector_List::operator== (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) { return *this == *sl; }
    else if (auto ss = Cast<Simple_Selector>(&rhs)) { return *this == *ss; }
    else if (auto cpx = Cast<Complex_Selector>(&rhs)) { return *this == *cpx; }
    else if (auto cpd = Cast<Compound_Selector>(&rhs)) { return *this == *cpd; }
    else if (auto ls = Cast<List>(&rhs)) { return *this == *ls; }
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Selector_List::operator< (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) { return *this < *sl; }
    else if (auto ss = Cast<Simple_Selector>(&rhs)) { return *this < *ss; }
    else if (auto cpx = Cast<Complex_Selector>(&rhs)) { return *this < *cpx; }
    else if (auto cpd = Cast<Compound_Selector>(&rhs)) { return *this < *cpd; }
    else if (auto ls = Cast<List>(&rhs)) { return *this < *ls; }
    throw std::runtime_error("invalid selector base classes to compare");
  }

  // Selector lists can be compared to comma lists
  bool Selector_List::operator== (const Expression& rhs) const
  {
    if (auto l = Cast<List>(&rhs)) { return *this == *l; }
    if (auto s = Cast<Selector>(&rhs)) { return *this == *s; }
    if (Cast<String>(&rhs) || Cast<Null>(&rhs)) { return false; }
    throw std::runtime_error("invalid selector base classes to compare");
  }

  // Selector lists can be compared to comma lists
  bool Selector_List::operator< (const Expression& rhs) const
  {
    if (auto l = Cast<List>(&rhs)) { return *this < *l; }
    if (auto s = Cast<Selector>(&rhs)) { return *this < *s; }
    if (Cast<String>(&rhs) || Cast<Null>(&rhs)) { return true; }
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Complex_Selector::operator== (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this == *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Complex_Selector::operator< (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this < *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Compound_Selector::operator== (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this == *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Compound_Selector::operator< (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this < *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Selector_Schema::operator== (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this == *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Selector_Schema::operator< (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this < *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Simple_Selector::operator== (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this == *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this == *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this == *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this == *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  bool Simple_Selector::operator< (const Selector& rhs) const
  {
    if (auto sl = Cast<Selector_List>(&rhs)) return *this < *sl;
    if (auto ss = Cast<Simple_Selector>(&rhs)) return *this < *ss;
    if (auto cs = Cast<Complex_Selector>(&rhs)) return *this < *cs;
    if (auto ch = Cast<Compound_Selector>(&rhs)) return *this < *ch;
    throw std::runtime_error("invalid selector base classes to compare");
  }

  /*#########################################################################*/
  /*#########################################################################*/

  bool Selector_List::operator< (const Selector_List& rhs) const
  {
    size_t l = rhs.length();
    if (length() < l) l = length();
    for (size_t i = 0; i < l; i ++) {
      if (*at(i) < *rhs.at(i)) return true;
      if (*at(i) != *rhs.at(i)) return false;
    }
    return false;
  }

  bool Selector_List::operator== (const Selector_List& rhs) const
  {
    if (&rhs == this) return true;
    if (rhs.length() != length()) return false;
    std::unordered_set<const Complex_Selector *, HashPtr, ComparePtrs> lhs_set;
    lhs_set.reserve(length());
    for (const Complex_Selector_Obj &element : elements()) {
      lhs_set.insert(element.ptr());
    }
    for (const Complex_Selector_Obj &element : rhs.elements()) {
        if (lhs_set.find(element.ptr()) == lhs_set.end()) return false;
    }
    return true;
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
    const Complex_Selector* l = this;
    const Complex_Selector* r = &rhs;
    Compound_Selector* l_h = NULL;
    Compound_Selector* r_h = NULL;
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
    const Complex_Selector* l = this;
    const Complex_Selector* r = &rhs;
    Compound_Selector* l_h = NULL;
    Compound_Selector* r_h = NULL;
    if (l) l_h = l->head();
    if (r) r_h = r->head();
    // process all tails
    while (true)
    {
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

  /*#########################################################################*/
  /*#########################################################################*/

  bool Selector_List::operator== (const Complex_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return rhs.empty();
    return *at(0) == rhs;
  }

  bool Selector_List::operator< (const Complex_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return !rhs.empty();
    return *at(0) < rhs;
  }

  bool Selector_List::operator== (const Compound_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return rhs.empty();
    return *at(0) == rhs;
  }

  bool Selector_List::operator< (const Compound_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return !rhs.empty();
    return *at(0) < rhs;
  }

  bool Selector_List::operator== (const Simple_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return rhs.empty();
    return *at(0) == rhs;
  }

  bool Selector_List::operator< (const Simple_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return !rhs.empty();
    return *at(0) < rhs;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Complex_Selector::operator== (const Selector_List& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return false;
    if (len == 0) return empty();
    return *this == *rhs.at(0);
  }

  bool Complex_Selector::operator< (const Selector_List& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return true;
    if (len == 0) return false;
    return *this < *rhs.at(0);
  }

  bool Complex_Selector::operator== (const Compound_Selector& rhs) const
  {
    if (tail()) return false;
    if (!head()) return rhs.empty();
    return *head() == rhs;
  }

  bool Complex_Selector::operator< (const Compound_Selector& rhs) const
  {
    if (tail()) return false;
    if (!head()) return !rhs.empty();
    return *head() < rhs;
  }

  bool Complex_Selector::operator== (const Simple_Selector& rhs) const
  {
    if (tail()) return false;
    if (!head()) return rhs.empty();
    return *head() == rhs;
  }

  bool Complex_Selector::operator< (const Simple_Selector& rhs) const
  {
    if (tail()) return false;
    if (!head()) return !rhs.empty();
    return *head() < rhs;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Compound_Selector::operator== (const Selector_List& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return false;
    if (len == 0) return empty();
    return *this == *rhs.at(0);
  }

  bool Compound_Selector::operator< (const Selector_List& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return true;
    if (len == 0) return false;
    return *this < *rhs.at(0);
  }

  bool Compound_Selector::operator== (const Complex_Selector& rhs) const
  {
    if (rhs.tail()) return false;
    if (!rhs.head()) return empty();
    return *this == *rhs.head();
  }

  bool Compound_Selector::operator< (const Complex_Selector& rhs) const
  {
    if (rhs.tail()) return true;
    if (!rhs.head()) return false;
    return *this < *rhs.head();
  }

  bool Compound_Selector::operator< (const Simple_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return rhs.empty();
    return *at(0) == rhs;
  }

  bool Compound_Selector::operator== (const Simple_Selector& rhs) const
  {
    size_t len = length();
    if (len > 1) return false;
    if (len == 0) return !rhs.empty();
    return *at(0) < rhs;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Simple_Selector::operator== (const Selector_List& rhs) const
  {
    return rhs.length() == 1 && *this == *rhs.at(0);
  }

  bool Simple_Selector::operator< (const Selector_List& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return true;
    if (len == 0) return false;
    return *this < *rhs.at(0);
  }

  bool Simple_Selector::operator== (const Complex_Selector& rhs) const
  {
    return !rhs.tail() && rhs.head() &&
           rhs.combinator() == Complex_Selector::ANCESTOR_OF &&
           *this == *rhs.head();
  }

  bool Simple_Selector::operator< (const Complex_Selector& rhs) const
  {
    if (rhs.tail()) return true;
    if (!rhs.head()) return false;
    return *this < *rhs.head();
  }

  bool Simple_Selector::operator== (const Compound_Selector& rhs) const
  {
    return rhs.length() == 1 && *this == *rhs.at(0);
  }

  bool Simple_Selector::operator< (const Compound_Selector& rhs) const
  {
    size_t len = rhs.length();
    if (len > 1) return true;
    if (len == 0) return false;
    return *this < *rhs.at(0);
  }

  /*#########################################################################*/
  /*#########################################################################*/

  bool Simple_Selector::operator== (const Simple_Selector& rhs) const
  {
    switch (simple_type()) {
      case ID_SEL: return (const Id_Selector&) *this == rhs; break;
      case TYPE_SEL: return (const Type_Selector&) *this == rhs; break;
      case CLASS_SEL: return (const Class_Selector&) *this == rhs; break;
      case PARENT_SEL: return (const Parent_Selector&) *this == rhs; break;
      case PSEUDO_SEL: return (const Pseudo_Selector&) *this == rhs; break;
      case WRAPPED_SEL: return (const Wrapped_Selector&) *this == rhs; break;
      case ATTRIBUTE_SEL: return (const Attribute_Selector&) *this == rhs; break;
      case PLACEHOLDER_SEL: return (const Placeholder_Selector&) *this == rhs; break;
    }
    return false;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Id_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Id_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Type_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Type_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Class_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Class_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Parent_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Parent_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Pseudo_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Pseudo_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Wrapped_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Wrapped_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Attribute_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Attribute_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  bool Placeholder_Selector::operator== (const Simple_Selector& rhs) const
  {
    auto sel = Cast<Placeholder_Selector>(&rhs);
    return sel ? *this == *sel : false;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Id_Selector::operator== (const Id_Selector& rhs) const
  {
    // ID has no namespacing
    return name() == rhs.name();
  }

  bool Type_Selector::operator== (const Type_Selector& rhs) const
  {
    return is_ns_eq(rhs) && name() == rhs.name();
  }

  bool Class_Selector::operator== (const Class_Selector& rhs) const
  {
    // Class has no namespacing
    return name() == rhs.name();
  }

  bool Parent_Selector::operator== (const Parent_Selector& rhs) const
  {
    // Parent has no namespacing
    return name() == rhs.name();
  }

  bool Pseudo_Selector::operator== (const Pseudo_Selector& rhs) const
  {
    std::string lname = name();
    std::string rname = rhs.name();
    if (is_pseudo_class_element(lname)) {
      if (rname[0] == ':' && rname[1] == ':') {
        lname = lname.substr(1, std::string::npos);
      }
    }
    // right hand is special pseudo (single colon)
    if (is_pseudo_class_element(rname)) {
      if (lname[0] == ':' && lname[1] == ':') {
        lname = lname.substr(1, std::string::npos);
      }
    }
    // Pseudo has no namespacing
    if (lname != rname) return false;
    String_Obj lhs_ex = expression();
    String_Obj rhs_ex = rhs.expression();
    if (rhs_ex && lhs_ex) return *lhs_ex == *rhs_ex;
    else return lhs_ex.ptr() == rhs_ex.ptr();
  }

  bool Wrapped_Selector::operator== (const Wrapped_Selector& rhs) const
  {
    // Wrapped has no namespacing
    if (name() != rhs.name()) return false;
    return *(selector()) == *(rhs.selector());
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

  bool Placeholder_Selector::operator== (const Placeholder_Selector& rhs) const
  {
    // Placeholder has no namespacing
    return name() == rhs.name();
  }

  /*#########################################################################*/
  /*#########################################################################*/

  bool Simple_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (simple_type()) {
      case ID_SEL: return (const Id_Selector&) *this < rhs; break;
      case TYPE_SEL: return (const Type_Selector&) *this < rhs; break;
      case CLASS_SEL: return (const Class_Selector&) *this < rhs; break;
      case PARENT_SEL: return (const Parent_Selector&) *this < rhs; break;
      case PSEUDO_SEL: return (const Pseudo_Selector&) *this < rhs; break;
      case WRAPPED_SEL: return (const Wrapped_Selector&) *this < rhs; break;
      case ATTRIBUTE_SEL: return (const Attribute_Selector&) *this < rhs; break;
      case PLACEHOLDER_SEL: return (const Placeholder_Selector&) *this < rhs; break;
    }
    return false;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Id_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case TYPE_SEL: return '#' < 's'; break;
      case CLASS_SEL: return '#' < '.'; break;
      case PARENT_SEL: return '#' < '&'; break;
      case PSEUDO_SEL: return '#' < ':'; break;
      case WRAPPED_SEL: return '#' < '('; break;
      case ATTRIBUTE_SEL: return '#' < '['; break;
      case PLACEHOLDER_SEL: return '#' < '%'; break;
      case ID_SEL: /* let if fall through */ break;
    }
    const Id_Selector& sel =
      (const Id_Selector&) rhs;
    return *this < sel;
  }

  bool Type_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return 'e' < '#'; break;
      case CLASS_SEL: return 'e' < '.'; break;
      case PARENT_SEL: return 'e' < '&'; break;
      case PSEUDO_SEL: return 'e' < ':'; break;
      case WRAPPED_SEL: return 'e' < '('; break;
      case ATTRIBUTE_SEL: return 'e' < '['; break;
      case PLACEHOLDER_SEL: return 'e' < '%'; break;
      case TYPE_SEL: /* let if fall through */ break;
    }
    const Type_Selector& sel =
      (const Type_Selector&) rhs;
    return *this < sel;
  }

  bool Class_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return '.' < '#'; break;
      case TYPE_SEL: return '.' < 's'; break;
      case PARENT_SEL: return '.' < '&'; break;
      case PSEUDO_SEL: return '.' < ':'; break;
      case WRAPPED_SEL: return '.' < '('; break;
      case ATTRIBUTE_SEL: return '.' < '['; break;
      case PLACEHOLDER_SEL: return '.' < '%'; break;
      case CLASS_SEL: /* let if fall through */ break;
    }
    const Class_Selector& sel =
      (const Class_Selector&) rhs;
    return *this < sel;
  }

  bool Pseudo_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return ':' < '#'; break;
      case TYPE_SEL: return ':' < 's'; break;
      case CLASS_SEL: return ':' < '.'; break;
      case PARENT_SEL: return ':' < '&'; break;
      case WRAPPED_SEL: return ':' < '('; break;
      case ATTRIBUTE_SEL: return ':' < '['; break;
      case PLACEHOLDER_SEL: return ':' < '%'; break;
      case PSEUDO_SEL: /* let if fall through */ break;
    }
    const Pseudo_Selector& sel =
      (const Pseudo_Selector&) rhs;
    return *this < sel;
  }

  bool Wrapped_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return '(' < '#'; break;
      case TYPE_SEL: return '(' < 's'; break;
      case CLASS_SEL: return '(' < '.'; break;
      case PARENT_SEL: return '(' < '&'; break;
      case PSEUDO_SEL: return '(' < ':'; break;
      case ATTRIBUTE_SEL: return '(' < '['; break;
      case PLACEHOLDER_SEL: return '(' < '%'; break;
      case WRAPPED_SEL: /* let if fall through */ break;
    }
    const Wrapped_Selector& sel =
      (const Wrapped_Selector&) rhs;
    return *this < sel;
  }

  bool Parent_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return '&' < '#'; break;
      case TYPE_SEL: return '&' < 's'; break;
      case CLASS_SEL: return '&' < '.'; break;
      case PSEUDO_SEL: return '&' < ':'; break;
      case WRAPPED_SEL: return '&' < '('; break;
      case ATTRIBUTE_SEL: return '&' < '['; break;
      case PLACEHOLDER_SEL: return '&' < '%'; break;
      case PARENT_SEL: /* let if fall through */ break;
    }
    const Parent_Selector& sel =
      (const Parent_Selector&) rhs;
    return *this < sel;
  }

  bool Attribute_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return '[' < '#'; break;
      case TYPE_SEL: return '[' < 'e'; break;
      case CLASS_SEL: return '[' < '.'; break;
      case PARENT_SEL: return '[' < '&'; break;
      case PSEUDO_SEL: return '[' < ':'; break;
      case WRAPPED_SEL: return '[' < '('; break;
      case PLACEHOLDER_SEL: return '[' < '%'; break;
      case ATTRIBUTE_SEL: /* let if fall through */ break;
    }
    const Attribute_Selector& sel =
      (const Attribute_Selector&) rhs;
    return *this < sel;
  }

  bool Placeholder_Selector::operator< (const Simple_Selector& rhs) const
  {
    switch (rhs.simple_type()) {
      case ID_SEL: return '%' < '#'; break;
      case TYPE_SEL: return '%' < 's'; break;
      case CLASS_SEL: return '%' < '.'; break;
      case PARENT_SEL: return '%' < '&'; break;
      case PSEUDO_SEL: return '%' < ':'; break;
      case WRAPPED_SEL: return '%' < '('; break;
      case ATTRIBUTE_SEL: return '%' < '['; break;
      case PLACEHOLDER_SEL: /* let if fall through */ break;
    }
    const Placeholder_Selector& sel =
      (const Placeholder_Selector&) rhs;
    return *this < sel;
  }

  /***************************************************************************/
  /***************************************************************************/

  bool Id_Selector::operator< (const Id_Selector& rhs) const
  {
    // ID has no namespacing
    return name() < rhs.name();
  }

  bool Type_Selector::operator< (const Type_Selector& rhs) const
  {
    return has_ns_ == rhs.has_ns_
      ? (ns_ == rhs.ns_
         ? name_ < rhs.name_
         : ns_ < rhs.ns_)
      : has_ns_ < rhs.has_ns_;
  }

  bool Class_Selector::operator< (const Class_Selector& rhs) const
  {
    // Class has no namespacing
    return name() < rhs.name();
  }

  bool Parent_Selector::operator< (const Parent_Selector& rhs) const
  {
    // Parent has no namespacing
    return name() < rhs.name();
  }

  bool Pseudo_Selector::operator< (const Pseudo_Selector& rhs) const
  {
    std::string lname = name();
    std::string rname = rhs.name();
    if (is_pseudo_class_element(lname)) {
      if (rname[0] == ':' && rname[1] == ':') {
        lname = lname.substr(1, std::string::npos);
      }
    }
    // right hand is special pseudo (single colon)
    if (is_pseudo_class_element(rname)) {
      if (lname[0] == ':' && lname[1] == ':') {
        lname = lname.substr(1, std::string::npos);
      }
    }
    // Peudo has no namespacing
    if (lname != rname)
    { return lname < rname; }
    String_Obj lhs_ex = expression();
    String_Obj rhs_ex = rhs.expression();
    if (rhs_ex && lhs_ex) return *lhs_ex < *rhs_ex;
    else return lhs_ex.ptr() < rhs_ex.ptr();
  }

  bool Wrapped_Selector::operator< (const Wrapped_Selector& rhs) const
  {
    // Wrapped has no namespacing
    if (name() != rhs.name())
    { return name() < rhs.name(); }
    return *(selector()) < *(rhs.selector());
  }

  bool Attribute_Selector::operator< (const Attribute_Selector& rhs) const
  {
    if (is_ns_eq(rhs)) {
      if (name() != rhs.name())
      { return name() < rhs.name(); }
      if (matcher() != rhs.matcher())
      { return matcher() < rhs.matcher(); }
      bool no_lhs_val = value().isNull();
      bool no_rhs_val = rhs.value().isNull();
      if (no_lhs_val && no_rhs_val) return false; // equal
      else if (no_lhs_val) return true; // lhs is null
      else if (no_rhs_val) return false; // rhs is null
      return *value() < *rhs.value(); // both are given
    } else { return ns() < rhs.ns(); }
  }

  bool Placeholder_Selector::operator< (const Placeholder_Selector& rhs) const
  {
    // Placeholder has no namespacing
    return name() < rhs.name();
  }

  /*#########################################################################*/
  /*#########################################################################*/

}
