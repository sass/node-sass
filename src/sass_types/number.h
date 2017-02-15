#ifndef SASS_TYPES_NUMBER_H
#define SASS_TYPES_NUMBER_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class Number : public SassValueWrapper<Number> {
    public:
      Number(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassNumber"; }
      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);

      static NAPI_METHOD(GetValue);
      static NAPI_METHOD(GetUnit);
      static NAPI_METHOD(SetValue);
      static NAPI_METHOD(SetUnit);
  };
}

#endif
