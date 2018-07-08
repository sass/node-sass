#ifndef SASS_TYPES_NULL_H
#define SASS_TYPES_NULL_H

#include "value.h"

// node-sass only builds with MSVC 2013 which doesn't appear to have char16_t defined
#define char16_t wchar_t

#include <napi.h>

namespace SassTypes
{
  class Null : public SassTypes::Value {
    public:
      static Null& get_singleton();
      static napi_value get_constructor(napi_env env);

      Sass_Value* get_sass_value();
      napi_value get_js_object(napi_env env);

      static napi_value New(napi_env env, napi_callback_info info);

    private:
      Null();

      static napi_value construct_and_wrap_instance(napi_env env, napi_value ctor, Null* n);

      napi_ref js_object;

      static napi_ref constructor;
      static bool constructor_locked;
  };
}

#endif
