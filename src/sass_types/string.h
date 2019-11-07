#ifndef SASS_TYPES_STRING_H
#define SASS_TYPES_STRING_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class String : public SassValueWrapper<String> {
    public:
      String(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassString"; }

      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);

      static napi_value GetValue(napi_env env, napi_callback_info info);
      static napi_value SetValue(napi_env env, napi_callback_info info);
  };
}

#endif
