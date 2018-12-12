#ifndef SASS_AST2C_H
#define SASS_AST2C_H

#include "ast_fwd_decl.hpp"
#include "operation.hpp"
#include "sass/values.h"

namespace Sass {

  class AST2C : public Operation_CRTP<union Sass_Value*, AST2C> {

  public:

    AST2C() { }
    ~AST2C() { }

    union Sass_Value* operator()(Boolean_Ptr);
    union Sass_Value* operator()(Number_Ptr);
    union Sass_Value* operator()(Color_RGBA_Ptr);
    union Sass_Value* operator()(Color_HSLA_Ptr);
    union Sass_Value* operator()(String_Constant_Ptr);
    union Sass_Value* operator()(String_Quoted_Ptr);
    union Sass_Value* operator()(Custom_Warning_Ptr);
    union Sass_Value* operator()(Custom_Error_Ptr);
    union Sass_Value* operator()(List_Ptr);
    union Sass_Value* operator()(Map_Ptr);
    union Sass_Value* operator()(Null_Ptr);
    union Sass_Value* operator()(Arguments_Ptr);
    union Sass_Value* operator()(Argument_Ptr);

    // return sass error if type is not supported
    union Sass_Value* fallback(AST_Node_Ptr x)
    { return sass_make_error("unknown type for C-API"); }

  };

}

#endif
