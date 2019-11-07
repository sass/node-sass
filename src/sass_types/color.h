#ifndef SASS_TYPES_COLOR_H
#define SASS_TYPES_COLOR_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class Color : public SassValueWrapper<Color> {
    public:
      Color(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassColor"; }
      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);

      static napi_value GetR(napi_env env, napi_callback_info info);
      static napi_value GetG(napi_env env, napi_callback_info info);
      static napi_value GetB(napi_env env, napi_callback_info info);
      static napi_value GetA(napi_env env, napi_callback_info info);
      static napi_value SetR(napi_env env, napi_callback_info info);
      static napi_value SetG(napi_env env, napi_callback_info info);
      static napi_value SetB(napi_env env, napi_callback_info info);
      static napi_value SetA(napi_env env, napi_callback_info info);
  };
}

#endif
