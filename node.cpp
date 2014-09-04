#include "node.hpp"
#include "to_string.hpp"


namespace Sass {


  Node Node::createCombinator(const Complex_Selector::Combinator& combinator) {
    NodeDequePtr null;
    return Node(COMBINATOR, combinator, NULL /*pSelector*/, null /*pCollection*/);
  }

    
  Node Node::createSelector(Complex_Selector* pSelector, Context& ctx) {
    NodeDequePtr null;
    return Node(NIL, Complex_Selector::ANCESTOR_OF, pSelector->clone(ctx), null /*pCollection*/);
  }

    
  Node Node::createCollection() {
    NodeDequePtr pEmptyCollection = make_shared<NodeDeque>();
    return Node(NIL, Complex_Selector::ANCESTOR_OF, NULL /*pSelector*/, pEmptyCollection);
  }

    
  Node Node::createCollection(const NodeDeque& values) {
    NodeDequePtr pShallowCopiedCollection = make_shared<NodeDeque>(values);
    return Node(NIL, Complex_Selector::ANCESTOR_OF, NULL /*pSelector*/, pShallowCopiedCollection);
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
    for (NodeDeque::iterator iter = mpCollection->begin(), iterEnd = mpCollection->end(); iter != iterEnd; iter++) {
      Node& toClone = *iter;
      pNewCollection->push_back(toClone.clone(ctx));
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

    	if (pToConvert->combinator() != Complex_Selector::ANCESTOR_OF) {
        node.collection()->push_back(Node::createCombinator(pToConvert->combinator()));
      }
      
      node.collection()->push_back(Node::createSelector(pToConvert, ctx));
      
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


    if (!childNodes.empty() && childNodes.front().isCombinator()) {
      throw "The first element in the selector must not be a combinator.";
    }
    
    if (!childNodes.empty() && childNodes.back().isCombinator()) {
      throw "The last element in the selector must not be a combinator.";
    }
    
    
    Complex_Selector* pCurrent = NULL;
    Complex_Selector* pTail = NULL; // We're looping backwards, so this is the element after pCurrent in the Complex_Selector* we're building up
    
    for (NodeDeque::reverse_iterator childIter = childNodes.rbegin(), childIterEnd = childNodes.rend(); childIter != childIterEnd; childIter++) {
      Node& child = *childIter;
      
      /*
      if (pCurrentCSOC.isCombinator()) {
        // You might suspect that
        throw "This should never happen. Either this algorithm is busted or there were two combinators in a row (which shouldn't happen).";
      }

      pCurrent = pCurrentCSOC.selector()->clone(ctx);
      

      
      if (index > 0) {
        const ComplexSelectorOrCombinator& pPreviousCSOC = toConvert[index - 1];
        
        if (pPreviousCSOC.isCombinator()) {
          pCurrent->combinator(pPreviousCSOC.combinator());
          index--; // skip over this combinator in our iteration now that we've consumed it
        }
      }
      
      pCurrent->tail(pTail);
      
			pTail = pCurrent;
      */
    }

    
    return pCurrent;
  }


}