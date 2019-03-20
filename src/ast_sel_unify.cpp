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

// #define DEBUG_UNIFY

namespace Sass {

  Compound_Selector* Compound_Selector::unify_with(Compound_Selector* rhs)
  {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(Compound[" + this->to_string() + "], Compound[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    if (empty()) return rhs;
    Compound_Selector_Obj unified = SASS_MEMORY_COPY(rhs);
    for (const Simple_Selector_Obj& sel : elements()) {
      unified = sel->unify_with(unified);
      if (unified.isNull()) break;
    }

    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = Compound[" << unified->to_string() << "]" << std::endl;
    #endif
    return unified.detach();
  }

  Compound_Selector* Simple_Selector::unify_with(Compound_Selector* rhs)
  {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(Simple[" + this->to_string() + "], Compound[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    if (rhs->length() == 1) {
      if (rhs->at(0)->is_universal()) {
        Compound_Selector* this_compound = SASS_MEMORY_NEW(Compound_Selector, pstate(), 1);
        this_compound->append(SASS_MEMORY_COPY(this));
        Compound_Selector* unified = rhs->at(0)->unify_with(this_compound);
        if (unified == nullptr || unified != this_compound) delete this_compound;

        #ifdef DEBUG_UNIFY
        std::cerr << "> " << debug_call << " = " << "Compound[" << unified->to_string() << "]" << std::endl;
        #endif
        return unified;
      }
    }
    for (const Simple_Selector_Obj& sel : rhs->elements()) {
      if (*this == *sel) {
        #ifdef DEBUG_UNIFY
        std::cerr << "> " << debug_call << " = " << "Compound[" << rhs->to_string() << "]" << std::endl;
        #endif
        return rhs;
      }
    }
    const int lhs_order = this->unification_order();
    size_t i = rhs->length();
    while (i > 0 && lhs_order < rhs->at(i - 1)->unification_order()) --i;
    rhs->insert(rhs->begin() + i, this);
    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = " << "Compound[" << rhs->to_string() << "]" << std::endl;
    #endif
    return rhs;
  }

  Simple_Selector* Type_Selector::unify_with(Simple_Selector* rhs)
  {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(Type[" + this->to_string() + "], Simple[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    bool rhs_ns = false;
    if (!(is_ns_eq(*rhs) || rhs->is_universal_ns())) {
      if (!is_universal_ns()) {
        #ifdef DEBUG_UNIFY
        std::cerr << "> " << debug_call << " = nullptr" << std::endl;
        #endif
        return nullptr;
      }
      rhs_ns = true;
    }
    bool rhs_name = false;
    if (!(name_ == rhs->name() || rhs->is_universal())) {
      if (!(is_universal())) {
        #ifdef DEBUG_UNIFY
        std::cerr << "> " << debug_call << " = nullptr" << std::endl;
        #endif
        return nullptr;
      }
      rhs_name = true;
    }
    if (rhs_ns) {
      ns(rhs->ns());
      has_ns(rhs->has_ns());
    }
    if (rhs_name) name(rhs->name());
    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = Simple[" << this->to_string() << "]" << std::endl;
    #endif
    return this;
  }

  Compound_Selector* Type_Selector::unify_with(Compound_Selector* rhs)
  {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(Type[" + this->to_string() + "], Compound[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    if (rhs->empty()) {
      rhs->append(this);
      #ifdef DEBUG_UNIFY
      std::cerr << "> " << debug_call << " = Compound[" << rhs->to_string() << "]" << std::endl;
      #endif
      return rhs;
    }
    Type_Selector* rhs_0 = Cast<Type_Selector>(rhs->at(0));
    if (rhs_0 != nullptr) {
      Simple_Selector* unified = unify_with(rhs_0);
      if (unified == nullptr) {
        #ifdef DEBUG_UNIFY
        std::cerr << "> " << debug_call << " = nullptr" << std::endl;
        #endif
        return nullptr;
      }
      rhs->elements()[0] = unified;
    } else if (!is_universal() || (has_ns_ && ns_ != "*")) {
      rhs->insert(rhs->begin(), this);
    }
    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = Compound[" << rhs->to_string() << "]" << std::endl;
    #endif
    return rhs;
  }

  Compound_Selector* Class_Selector::unify_with(Compound_Selector* rhs)
  {
    rhs->has_line_break(has_line_break());
    return Simple_Selector::unify_with(rhs);
  }

  Compound_Selector* Id_Selector::unify_with(Compound_Selector* rhs)
  {
    for (const Simple_Selector_Obj& sel : rhs->elements()) {
      if (Id_Selector* id_sel = Cast<Id_Selector>(sel)) {
        if (id_sel->name() != name()) return nullptr;
      }
    }
    rhs->has_line_break(has_line_break());
    return Simple_Selector::unify_with(rhs);
  }

  Compound_Selector* Pseudo_Selector::unify_with(Compound_Selector* rhs)
  {
    if (is_pseudo_element()) {
      for (const Simple_Selector_Obj& sel : rhs->elements()) {
        if (Pseudo_Selector* pseudo_sel = Cast<Pseudo_Selector>(sel)) {
          if (pseudo_sel->is_pseudo_element() && pseudo_sel->name() != name()) return nullptr;
        }
      }
    }
    return Simple_Selector::unify_with(rhs);
  }

  Selector_List* Complex_Selector::unify_with(Complex_Selector* rhs)
  {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(Complex[" + this->to_string() + "], Complex[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    // get last tails (on the right side)
    Complex_Selector* l_last = this->mutable_last();
    Complex_Selector* r_last = rhs->mutable_last();

    // check valid pointers (assertion)
    SASS_ASSERT(l_last, "lhs is null");
    SASS_ASSERT(r_last, "rhs is null");

    // Not sure about this check, but closest way I could check
    // was to see if this is a ruby 'SimpleSequence' equivalent.
    // It seems to do the job correctly as some specs react to this
    if (l_last->combinator() != Combinator::ANCESTOR_OF) return nullptr;
    if (r_last->combinator() != Combinator::ANCESTOR_OF) return nullptr;

    // get the headers for the last tails
    Compound_Selector* l_last_head = l_last->head();
    Compound_Selector* r_last_head = r_last->head();

    // check valid head pointers (assertion)
    SASS_ASSERT(l_last_head, "lhs head is null");
    SASS_ASSERT(r_last_head, "rhs head is null");

    // get the unification of the last compound selectors
    Compound_Selector_Obj unified = r_last_head->unify_with(l_last_head);

    // abort if we could not unify heads
    if (unified == nullptr) return nullptr;

    // move the head
    if (l_last_head->is_universal()) l_last->head({});
    r_last->head(unified);

    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " before weave: lhs=" << this->to_string() << " rhs=" << rhs->to_string() << std::endl;
    #endif

    // create nodes from both selectors
    Node lhsNode = complexSelectorToNode(this);
    Node rhsNode = complexSelectorToNode(rhs);

    // Complex_Selector_Obj fake = unified->to_complex();
    // Node unified_node = complexSelectorToNode(fake);
    // // add to permutate the list?
    // rhsNode.plus(unified_node);

    // do some magic we inherit from node and extend
    Node node = subweave(lhsNode, rhsNode);
    Selector_List_Obj result = SASS_MEMORY_NEW(Selector_List, pstate(), node.collection()->size());
    for (auto &item : *node.collection()) {
      result->append(nodeToComplexSelector(Node::naiveTrim(item)));
    }

    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = " << result->to_string() << std::endl;
    #endif

    // only return if list has some entries
    return result->length() ? result.detach() : nullptr;
  }

  Selector_List* Selector_List::unify_with(Selector_List* rhs) {
    #ifdef DEBUG_UNIFY
    const std::string debug_call = "unify(List[" + this->to_string() + "], List[" + rhs->to_string() + "])";
    std::cerr << debug_call << std::endl;
    #endif

    std::vector<Complex_Selector_Obj> result;
    // Unify all of children with RHS's children, storing the results in `unified_complex_selectors`
    for (Complex_Selector_Obj& seq1 : elements()) {
      for (Complex_Selector_Obj& seq2 : rhs->elements()) {
        Complex_Selector_Obj seq1_copy = SASS_MEMORY_CLONE(seq1);
        Complex_Selector_Obj seq2_copy = SASS_MEMORY_CLONE(seq2);
        Selector_List_Obj unified = seq1_copy->unify_with(seq2_copy);
        if (unified) {
          result.reserve(result.size() + unified->length());
          std::copy(unified->begin(), unified->end(), std::back_inserter(result));
        }
      }
    }

    // Creates the final Selector_List by combining all the complex selectors
    Selector_List* final_result = SASS_MEMORY_NEW(Selector_List, pstate(), result.size());
    for (Complex_Selector_Obj& sel : result) {
      final_result->append(sel);
    }
    #ifdef DEBUG_UNIFY
    std::cerr << "> " << debug_call << " = " << final_result->to_string() << std::endl;
    #endif
    return final_result;
  }

}
