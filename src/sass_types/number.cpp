#include "number.h"
#include "../create_string.h"

namespace SassTypes
{
  Number::Number(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* Number::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    double value = 0;
    char const* unit = "";

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));

      if (t != napi_number) {
        return fail("First argument should be a number.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[0], &value));

      if (raw_val.size() >= 2) {
        CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[1], &t));

        if (t != napi_string) {
          return fail("Second argument should be a string.", out);
        }

        unit = create_string(env, raw_val[1]);
      }
    }

    return *out = sass_make_number(value, unit);
  }

  napi_value Number::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    napi_property_descriptor descriptors [] = {
        { "getValue", GetValue },
        { "getUnit", GetUnit },
        { "setValue", SetValue },
        { "setUnit", SetUnit },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), cb, nullptr, 4, descriptors, &ctor));
    return ctor;
  }

  NAPI_METHOD(Number::GetValue) {
    CommonGetNumber(env, info, sass_number_get_value);
  }

  NAPI_METHOD(Number::GetUnit) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    const char* s = sass_number_get_unit(unwrap(env, _this)->value);
    int len = (int)strlen(s);

    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, s, len, &str));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, str));
  }

  NAPI_METHOD(Number::SetValue) {
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

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a number"));
      return;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    auto v = unwrap(env, _this)->value;
    double d;
    CHECK_NAPI_RESULT(napi_get_value_double(env, argv, &d));

    sass_number_set_value(v, d);
  }

  NAPI_METHOD(Number::SetUnit) {
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

    sass_number_set_unit(v, s);
  }
}
