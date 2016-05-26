#ifndef SASS_EXTEND_H
#define SASS_EXTEND_H

#include <string>
#include <set>

#include "ast.hpp"
#include "operation.hpp"
#include "subset_map.hpp"

namespace Sass {

  class Context;
  class Node;

  typedef Subset_Map<std::string, std::pair<Sequence_Selector*, SimpleSequence_Selector*> > ExtensionSubsetMap;

  class Extend : public Operation_CRTP<void, Extend> {

    Context&            ctx;
    ExtensionSubsetMap& subset_map;

    void fallback_impl(AST_Node* n) { }

  public:
    static Node subweave(Node& one, Node& two, Context& ctx);
    static CommaSequence_Selector* extendSelectorList(CommaSequence_Selector* pSelectorList, Context& ctx, ExtensionSubsetMap& subset_map, bool isReplace, bool& extendedSomething, std::set<SimpleSequence_Selector>& seen);
    static CommaSequence_Selector* extendSelectorList(CommaSequence_Selector* pSelectorList, Context& ctx, ExtensionSubsetMap& subset_map, bool isReplace, bool& extendedSomething);
    static CommaSequence_Selector* extendSelectorList(CommaSequence_Selector* pSelectorList, Context& ctx, ExtensionSubsetMap& subset_map, bool isReplace = false) {
      bool extendedSomething = false;
      return extendSelectorList(pSelectorList, ctx, subset_map, isReplace, extendedSomething);
    }
    static CommaSequence_Selector* extendSelectorList(CommaSequence_Selector* pSelectorList, Context& ctx, ExtensionSubsetMap& subset_map, std::set<SimpleSequence_Selector>& seen) {
      bool isReplace = false;
      bool extendedSomething = false;
      return extendSelectorList(pSelectorList, ctx, subset_map, isReplace, extendedSomething, seen);
    }
    Extend(Context&, ExtensionSubsetMap&);
    ~Extend() { }

    void operator()(Block*);
    void operator()(Ruleset*);
    void operator()(Supports_Block*);
    void operator()(Media_Block*);
    void operator()(Directive*);

    template <typename U>
    void fallback(U x) { return fallback_impl(x); }
  };

}

#endif
