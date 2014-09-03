#include "node.hpp"

namespace Sass {


  Node Node::createCombinator(Complex_Selector* pSelector) {
    NodeDequePtr null;
    return Node(COMBINATOR, pSelector->combinator(), NULL /*pSelector*/, null /*pCollection*/);
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


}