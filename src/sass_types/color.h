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

      static NAPI_METHOD(GetR);
      static NAPI_METHOD(GetG);
      static NAPI_METHOD(GetB);
      static NAPI_METHOD(GetA);
      static NAPI_METHOD(SetR);
      static NAPI_METHOD(SetG);
      static NAPI_METHOD(SetB);
      static NAPI_METHOD(SetA);
  };
}

#endif
