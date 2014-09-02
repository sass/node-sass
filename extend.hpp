#define SASS_EXTEND

#include <vector>
#include <map>
#include <set>
#include <iostream>

#ifndef SASS_AST
#include "ast.hpp"
#endif

#ifndef SASS_OPERATION
#include "operation.hpp"
#endif

#ifndef SASS_SUBSET_MAP
#include "subset_map.hpp"
#endif

namespace Sass {
  using namespace std;

  struct Context;
  struct Backtrace;
  
  typedef multimap<Compound_Selector, Complex_Selector*> Extensions;
  typedef Subset_Map<string, pair<Complex_Selector*, Compound_Selector*> > ExtensionSubsetMap;

  class Extend : public Operation_CRTP<void, Extend> {

    Context&            ctx;
    Extensions&         extensions;
    ExtensionSubsetMap& subset_map;

    Backtrace*          backtrace;

    void fallback_impl(AST_Node* n) { };

  public:
    Extend(Context&, Extensions&, ExtensionSubsetMap&, Backtrace*);
    virtual ~Extend() { }

    using Operation<void>::operator();

    void operator()(Block*);
    void operator()(Ruleset*);
    void operator()(Media_Block*);
    void operator()(At_Rule*);

    template <typename U>
    void fallback(U x) { return fallback_impl(x); }
  };

}
