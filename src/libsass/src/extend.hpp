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

  class Extend : public Operation_CRTP<void, Extend> {

    Context&            ctx;
    Subset_Map& subset_map;

    void fallback_impl(AST_Node_Ptr n) { }

  public:
    static Node subweave(Node& one, Node& two, Context& ctx);
    static Selector_List_Ptr extendSelectorList(Selector_List_Obj pSelectorList, Context& ctx, Subset_Map& subset_map, bool isReplace, bool& extendedSomething, std::set<Compound_Selector>& seen);
    static Selector_List_Ptr extendSelectorList(Selector_List_Obj pSelectorList, Context& ctx, Subset_Map& subset_map, bool isReplace, bool& extendedSomething);
    static Selector_List_Ptr extendSelectorList(Selector_List_Obj pSelectorList, Context& ctx, Subset_Map& subset_map, bool isReplace = false) {
      bool extendedSomething = false;
      return extendSelectorList(pSelectorList, ctx, subset_map, isReplace, extendedSomething);
    }
    static Selector_List_Ptr extendSelectorList(Selector_List_Obj pSelectorList, Context& ctx, Subset_Map& subset_map, std::set<Compound_Selector>& seen) {
      bool isReplace = false;
      bool extendedSomething = false;
      return extendSelectorList(pSelectorList, ctx, subset_map, isReplace, extendedSomething, seen);
    }
    Extend(Context&, Subset_Map&);
    ~Extend() { }

    void operator()(Block_Ptr);
    void operator()(Ruleset_Ptr);
    void operator()(Supports_Block_Ptr);
    void operator()(Media_Block_Ptr);
    void operator()(Directive_Ptr);

    template <typename U>
    void fallback(U x) { return fallback_impl(x); }
  };

}

#endif
