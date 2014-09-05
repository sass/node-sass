#include "node.hpp"
#include "to_string.hpp"
#include "context.hpp"

namespace Sass {


  Node Node::createCombinator(const Complex_Selector::Combinator& combinator) {
    NodeDequePtr null;
    return Node(COMBINATOR, combinator, NULL /*pSelector*/, null /*pCollection*/);
  }

    
  Node Node::createSelector(Complex_Selector* pSelector, Context& ctx) {
    NodeDequePtr null;
    return Node(SELECTOR, Complex_Selector::ANCESTOR_OF, pSelector->clone(ctx), null /*pCollection*/);
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
    
    bool first = true;
    while (pToConvert) {

      // the first Complex_Selector contains a dummy head pointer, skip it.
      if (!first && pToConvert->head() != NULL) {
        node.collection()->push_back(Node::createSelector(pToConvert, ctx));
      }

      if (pToConvert->combinator() != Complex_Selector::ANCESTOR_OF) {
        node.collection()->push_back(Node::createCombinator(pToConvert->combinator()));
      }

      pToConvert = pToConvert->tail();
      first = false;
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
        pCurrent->tail(child.selector());
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


}