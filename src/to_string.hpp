#ifndef SASS_TO_STRING_H
#define SASS_TO_STRING_H

#include <string>

#include "operation.hpp"

namespace Sass {

  class Context;
  class Null;

  class To_String : public Operation_CRTP<std::string, To_String> {
    // import all the class-specific methods and override as desired
    using Operation<std::string>::operator();
    // override this to define a catch-all
    std::string fallback_impl(AST_Node* n);

    Context* ctx;
    bool in_declaration;
    bool in_debug;

  public:

    To_String(Context* ctx = 0, bool in_declaration = true, bool in_debug = false);
    virtual ~To_String();

    std::string operator()(Null* n);
    std::string operator()(String_Schema*);
    std::string operator()(String_Quoted*);
    std::string operator()(String_Constant*);

    template <typename U>
    std::string fallback(U n) { return fallback_impl(n); }
  };
}

#endif
