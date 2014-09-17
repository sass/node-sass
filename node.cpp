#include "node.hpp"
#include "to_string.hpp"
#include "context.hpp"
#include "parser.hpp"

namespace Sass {


  Node Node::createCombinator(const Complex_Selector::Combinator& combinator) {
    NodeDequePtr null;
    return Node(COMBINATOR, combinator, NULL /*pSelector*/, null /*pCollection*/);
  }

    
  Node Node::createSelector(Complex_Selector* pSelector, Context& ctx) {
    NodeDequePtr null;

    Complex_Selector* pStripped = pSelector->clone(ctx);
    pStripped->tail(NULL);
    pStripped->combinator(Complex_Selector::ANCESTOR_OF);
    
    return Node(SELECTOR, Complex_Selector::ANCESTOR_OF, pStripped, null /*pCollection*/);
  }

    
  Node Node::createCollection() {
    NodeDequePtr pEmptyCollection = make_shared<NodeDeque>();
    return Node(COLLECTION, Complex_Selector::ANCESTOR_OF, NULL /*pSelector*/, pEmptyCollection);
  }

    
  Node Node::createCollection(const NodeDeque& values) {
    NodeDequePtr pShallowCopiedCollection = make_shared<NodeDeque>(values);
    return Node(COLLECTION, Complex_Selector::ANCESTOR_OF, NULL /*pSelector*/, pShallowCopiedCollection);
  }

    
  Node Node::createNil() {
    NodeDequePtr null;
    return Node(NIL, Complex_Selector::ANCESTOR_OF, NULL /*pSelector*/, null /*pCollection*/);
  }

  
  Node::Node(const TYPE& type, Complex_Selector::Combinator combinator, Complex_Selector* pSelector, NodeDequePtr& pCollection) :
  	mType(type),
    mCombinator(combinator),
    mpSelector(pSelector),
    mpCollection(pCollection) {}

  
  Node Node::clone(Context& ctx) const {
    NodeDequePtr pNewCollection = make_shared<NodeDeque>();
    if (mpCollection) {
      for (NodeDeque::iterator iter = mpCollection->begin(), iterEnd = mpCollection->end(); iter != iterEnd; iter++) {
        Node& toClone = *iter;
        pNewCollection->push_back(toClone.clone(ctx));
      }
    }
    
    return Node(mType, mCombinator, mpSelector ? mpSelector->clone(ctx) : NULL, pNewCollection);
  }
  

  bool Node::operator==(const Node& rhs) const {
    if (this->mType != rhs.mType) {
      return false;
    }
    
    if (this->isCombinator()) {

    	return this->combinator() == rhs.combinator();

    } else if (this->isNil()) {

      return true; // no state to check

    } else if (this->isSelector()){

    	return (!(*this->selector() < *rhs.selector()) && !(*rhs.selector() < *this->selector()));

    } else if (this->isCollection()) {

      if (this->collection()->size() != rhs.collection()->size()) {
        return false;
      }

      for (NodeDeque::iterator thisIter = mpCollection->begin(), thisIterEnd = mpCollection->end(),
           rhsIter = rhs.collection()->begin(); thisIter != thisIterEnd; thisIter++, rhsIter++) {

        if (*thisIter != *rhsIter) {
          return false;
        }

      }

      return true;

    }
    
    // We shouldn't get here. If we did, a new type was added, but this wasn't updated.
    return false;
  }
  
  
  ostream& operator<<(ostream& os, const Node& node) {

    if (node.isCombinator()) {

      switch (node.combinator()) {
        case Complex_Selector::ANCESTOR_OF: os << "\" \""; break;
        case Complex_Selector::PARENT_OF:   os << "\">\""; break;
        case Complex_Selector::PRECEDES:    os << "\"~\""; break;
        case Complex_Selector::ADJACENT_TO: os << "\"+\""; break;
      }
      
    } else if (node.isNil()) {
      
      os << "nil";
      
    } else if (node.isSelector()){
      
      To_String to_string;
      os << node.selector()->head()->perform(&to_string);
      
    } else if (node.isCollection()) {
      
			os << "[";
      
      for (NodeDeque::iterator iter = node.collection()->begin(), iterBegin = node.collection()->begin(), iterEnd = node.collection()->end(); iter != iterEnd; iter++) {
        if (iter != iterBegin) {
          os << ", ";
        }

				os << (*iter);
      }
      
      os << "]";
      
    }
    
    return os;

  }
  
  
  Node complexSelectorToNode(Complex_Selector* pToConvert, Context& ctx) {
    if (pToConvert == NULL) {
      return Node::createNil();
    }

		Node node = Node::createCollection();
    
    while (pToConvert) {

      // the first Complex_Selector may contain a dummy head pointer, skip it.
      if (pToConvert->head() != NULL && !pToConvert->head()->is_empty_reference()) {
        node.collection()->push_back(Node::createSelector(pToConvert, ctx));
      }

      if (pToConvert->combinator() != Complex_Selector::ANCESTOR_OF) {
        node.collection()->push_back(Node::createCombinator(pToConvert->combinator()));
      }

      pToConvert = pToConvert->tail();
    }
    
    return node;
  }


  Complex_Selector* nodeToComplexSelector(const Node& toConvert, Context& ctx) {
    if (toConvert.isNil()) {
      return NULL;
    }


    if (!toConvert.isCollection()) {
      throw "The node to convert to a Complex_Selector* must be a collection type or nil.";
    }
    

    NodeDeque& childNodes = *toConvert.collection();
    
    string noPath("");
    Position noPosition;
    Complex_Selector* pFirst = new (ctx.mem) Complex_Selector(noPath, noPosition, Complex_Selector::ANCESTOR_OF, NULL, NULL);
    Complex_Selector* pCurrent = pFirst;
    
    for (NodeDeque::iterator childIter = childNodes.begin(), childIterEnd = childNodes.end(); childIter != childIterEnd; childIter++) {

      Node& child = *childIter;
      
      if (child.isSelector()) {
        pCurrent->tail(child.selector()->clone(ctx));   // JMA - need to clone the selector, because they can end up getting shared across Node collections, and can result in an infinite loop during the call to parentSuperselector()
        pCurrent = pCurrent->tail();
      } else if (child.isCombinator()) {
        pCurrent->combinator(child.combinator());
        
        // if the next node is also a combinator, create another Complex_Selector to hold it so it doesn't replace the current combinator
        if (childIter+1 != childIterEnd) {
          Node& nextNode = *(childIter+1);
          if (nextNode.isCombinator()) {
            pCurrent->tail(new (ctx.mem) Complex_Selector(noPath, noPosition, Complex_Selector::ANCESTOR_OF, NULL, NULL));
            pCurrent = pCurrent->tail();
          }
        }
      } else {
        throw "The node to convert's children must be only combinators or selectors.";
      }
    }

    // Put the dummy Compound_Selector in the first position, for consistency with the rest of libsass
    Compound_Selector* fakeHead = new (ctx.mem) Compound_Selector(noPath, noPosition, 1);
    Selector_Reference* selectorRef = new (ctx.mem) Selector_Reference(noPath, noPosition, NULL);
    fakeHead->elements().push_back(selectorRef);
    pFirst->head(fakeHead);
    
    return pFirst;
  }

  // JMA - this is a helper function used by unify below.
  // sel - the next Node to unify (must be a selector node)
  // mergedTypes - should contain only the first type encountered, all subsequent types are compared against this and the unify is aborted if they don't match.
  // mergedQualifiers - are just appended as they're encountered
  bool unifySelector(const Node& sel, string& mergedTypes, stringstream& mergedQualifiers) {
    Compound_Selector* pSel = sel.selector()->head();
    for (size_t i=0; i<pSel->length(); i++) {
      Simple_Selector* next = pSel->elements()[i];
      if (typeid(*next) == typeid(Type_Selector)) {
        if (mergedTypes.length() == 0) {
          mergedTypes = ((Type_Selector*)next)->name();
        }
        else {
          // ensure that it matches, can't combine multiple different type selectors
          if (mergedTypes.compare(((Type_Selector*)next)->name()) != 0) {
            return false;
          }
        }
      }
      else if (typeid(*next) == typeid(Selector_Qualifier)) {
        // just append each selector qualifier        // TODO: is it necessary to filter duplicates?
        mergedQualifiers << ((Selector_Qualifier*)next)->name();
      }
      else {
        // TODO: only Type_Selector and Selector_Qualifier are handled by this implementation.  are any others valid?
        return false;
      }
    }
    return true;
  }
  
  // JMA - this is a helper function used by unify below.
  // sel - the next Node to unify
  // mergedTypes - should contain only the first type encountered, all subsequent types are compared against this and the unify is aborted if they don't match.
  // mergedQualifiers - are just appended as they're encountered
  bool unifyNode(const Node& sel, string& mergedTypes, stringstream& mergedQualifiers) {
    if (sel.isSelector()) {
      if (!unifySelector(sel, mergedTypes, mergedQualifiers)) {
        return false;
      }
    }
    else if (sel.isCollection() && sel.collection()->size() > 0) {
      for (NodeDeque::iterator iter = sel.collection()->begin(), endIter = sel.collection()->end(); iter != endIter; ++iter) {
        const Node& childNode = *iter;
        if (!unifyNode(childNode, mergedTypes, mergedQualifiers)) {
          return false;
        }
      }
    }
    else {
      // only compound selectors need to be handled, per the documentation
      return false;
    }
    
    return true;
  }
  
  // From sass documentation:
  // An operation unify(Compound Selector, Compound Selector) => Compound Selector that returns a selector that matches exactly those elements matched by both input selectors. For example, unify(.foo, .bar) returns .foo.bar. This only needs to work for compound or simpler selectors. This operation can fail (e.g. unify(a, h1)), in which case it should return null.
  // Example: sel1 = ['.a'] sel2 = ['.b'] merged = ['.b.a']
  // Example: sel1 = ['.a', '.b'] sel2 = ['.c', '.d'] merged = ['.c.d.a.b']
  // Example: sel1 = ['a', '.a'] sel2 = ['b', '.b'] merged = nil
  // Example: sel1 = ['x', '.a'] sel2 = ['x', '.b'] merged = ['x.a.b']
  Node unify(const Node& sel1, const Node& sel2, Context& ctx) {

    Node merged = Node::createCollection();

    string mergedTypes;
    stringstream mergedQualifiers;
    
    if (!unifyNode(sel2, mergedTypes, mergedQualifiers)) {
      return merged;
    }

    if (!unifyNode(sel1, mergedTypes, mergedQualifiers)) {
      return merged;
    }
    
    stringstream mergedSelector;
    mergedSelector << mergedTypes << mergedQualifiers.str() << ";";
    Complex_Selector* pComplexSelector = (*Parser::from_c_str(mergedSelector.str().c_str(), ctx, "", Position()).parse_selector_group())[0];
    merged.collection()->push_back(Node::createSelector(pComplexSelector->tail(), ctx));
    merged.collection()->push_back(Node::createCombinator(Complex_Selector::PRECEDES));
    return merged;
  }
  

}
