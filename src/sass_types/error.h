#ifndef SASS_TYPES_ERROR_H
#define SASS_TYPES_ERROR_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class Error : public SassValueWrapper<Error> {
    public:
      Error(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassError"; }
      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);
  };
}

#endif
