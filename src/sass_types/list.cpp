#include "list.h"

namespace SassTypes
{
  List::List(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* List::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    uint32_t length = 0;
    bool comma = true;
    bool is_bracketed = false;

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));

      if (t != napi_number) {
        return fail("First argument should be an integer.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_uint32(env, raw_val[0], &length));

      if (raw_val.size() >= 2) {
        CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[1], &t));

        if (t != napi_boolean) {
          return fail("Second argument should be a boolean.", out);
        }

        CHECK_NAPI_RESULT(napi_get_value_bool(env, raw_val[1], &comma));
      }
    }

    return *out = sass_make_list(length, comma ? SASS_COMMA : SASS_SPACE, is_bracketed);
  }

  napi_value List::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    napi_property_descriptor descriptors[] = {
      { "getLength", GetLength },
      { "getSeparator", GetSeparator },
      { "setSeparator", SetSeparator },
      { "getValue", GetValue },
      { "setValue", SetValue },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), cb, nullptr, 5, descriptors, &ctor));
    return ctor;
  }

  NAPI_METHOD(List::GetValue) {
    CommonGetIndexedValue(env, info, sass_list_get_length, sass_list_get_value);
  }

  NAPI_METHOD(List::SetValue) {
    CommonSetIndexedValue(env, info, sass_list_set_value);
  }

  NAPI_METHOD(List::GetSeparator) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    bool v = sass_list_get_separator(unwrap(env, _this)->value) == SASS_COMMA;
    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_boolean(env, v, &ret));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, ret));
  }

  NAPI_METHOD(List::SetSeparator) {
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

    if (t != napi_boolean) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a boolean"));
      return;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    bool b;
    CHECK_NAPI_RESULT(napi_get_value_bool(env, argv, &b));

    sass_list_set_separator(unwrap(env, _this)->value, b ? SASS_COMMA : SASS_SPACE);
  }

  NAPI_METHOD(List::GetLength) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    size_t s = sass_list_get_length(unwrap(env, _this)->value);
    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_number(env, (double)s, &ret));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, ret));
  }
}
