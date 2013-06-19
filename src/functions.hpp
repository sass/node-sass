#define SASS_FUNCTIONS

#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
#endif

#include <string>

namespace Sass {
  class Context;
  class AST_Node;
  class Expression;
  class Definition;
  typedef Environment<AST_Node*> Env;
  typedef const char* Signature;
  typedef Expression* (*Native_Function)(Env&, Context&, Signature, string, size_t);

  Definition* make_native_function(Signature, Native_Function, Context&);

  namespace Functions {


  }
}