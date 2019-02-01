#ifndef SASS_LISTIZE_H
#define SASS_LISTIZE_H

// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include <vector>
#include <iostream>

#include "ast.hpp"
#include "context.hpp"
#include "operation.hpp"
#include "environment.hpp"

namespace Sass {

  struct Backtrace;

  class Listize : public Operation_CRTP<Expression_Ptr, Listize> {

  public:
    Listize();
    ~Listize() { }

    Expression_Ptr operator()(Selector_List_Ptr);
    Expression_Ptr operator()(Complex_Selector_Ptr);
    Expression_Ptr operator()(Compound_Selector_Ptr);

    // generic fallback
    template <typename U>
    Expression_Ptr fallback(U x)
    { return Cast<Expression>(x); }
  };

}

#endif
