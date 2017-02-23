#include <nan.h>
#include "string.h"
#include "../create_string.h"

namespace SassTypes
{
  String::String(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* String::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    char const* value = "";

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));

      if (t != napi_string) {
        return fail("First argument should be a string.", out);
      }

      value = create_string(env, raw_val[0]);
    }

    return *out = sass_make_string(value);
  }

  napi_value String::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    napi_property_descriptor descriptors[] = {
      { "getValue", GetValue },
      { "setValue", SetValue },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), cb, nullptr, 2, descriptors, &ctor));
    return ctor;
  }

  NAPI_METHOD(String::GetValue) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    const char* v = sass_string_get_value(unwrap(env, _this)->value);
    int len = (int)strlen(v);

    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, v, len, &str));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, str));
  }

  NAPI_METHOD(String::SetValue) {
    int argLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argLength));

    if (argLength != 1) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected just one argument"));
      return;
    }

    napi_value argv;
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &argv, 1));
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv, &t));

    if (t != napi_string) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a string"));
      return;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    auto v = unwrap(env, _this)->value;
    char* s = create_string(env, argv);

    sass_string_set_value(v, s);
  }
}
