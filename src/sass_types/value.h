#ifndef SASS_TYPES_VALUE_H
#define SASS_TYPES_VALUE_H

#include <sass/values.h>
#include "../common.h"
#include <napi.h>

namespace SassTypes
{
  // This is the interface that all sass values must comply with
  class Value {
    public:
      virtual Sass_Value* get_sass_value() =0;
      virtual napi_value get_js_object(napi_env env) = 0;
  };
}

#endif
