#ifndef SASS_TYPES_MAP_H
#define SASS_TYPES_MAP_H

#include "sass_value_wrapper.h"

namespace SassTypes
{
  class Map : public SassValueWrapper<Map> {
    public:
      Map(napi_env, Sass_Value*);
      static char const* get_constructor_name() { return "SassMap"; }
      static Sass_Value* construct(napi_env, const std::vector<napi_value>, Sass_Value **);
      static napi_value getConstructor(napi_env, napi_callback);

      static NAPI_METHOD(GetValue);
      static NAPI_METHOD(SetValue);
      static NAPI_METHOD(GetKey);
      static NAPI_METHOD(SetKey);
      static NAPI_METHOD(GetLength);
  };
}

#endif
