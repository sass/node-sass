#ifndef SASS_TYPES_LIST_H
#define SASS_TYPES_LIST_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class List : public SassValueWrapper<List> {
    public:
      List(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassList"; }
      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);

      static NAPI_METHOD(GetValue);
      static NAPI_METHOD(SetValue);
      static NAPI_METHOD(GetSeparator);
      static NAPI_METHOD(SetSeparator);
      static NAPI_METHOD(GetLength);
  };
}

#endif
