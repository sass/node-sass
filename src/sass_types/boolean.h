#ifndef SASS_TYPES_BOOLEAN_H
#define SASS_TYPES_BOOLEAN_H

#include "value.h"
#include "sass_value_wrapper.h"
#include <node_api_helpers.h>

namespace SassTypes
{
  class Boolean : public SassTypes::Value {
    public:
      static Boolean& get_singleton(bool);
      static napi_value get_constructor(napi_env env);

      Sass_Value* get_sass_value();
      napi_value get_js_object(napi_env env);

      static NAPI_METHOD(New);
      static NAPI_METHOD(GetValue);

    private:
      Boolean(bool);

      static napi_value construct_and_wrap_instance(napi_env env, napi_value ctor, Boolean* b);

      bool value;
      napi_ref js_object;

      static napi_ref constructor;
      static bool constructor_locked;
  };
}

#endif
