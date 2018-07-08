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
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));

      if (t != napi_number) {
        return fail("First argument should be an integer.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_uint32(env, raw_val[0], &length));

      if (raw_val.size() >= 2) {
        CHECK_NAPI_RESULT(napi_typeof(env, raw_val[1], &t));

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
      { "getLength", nullptr, GetLength },
      { "getSeparator", nullptr, GetSeparator },
      { "setSeparator", nullptr, SetSeparator },
      { "getValue", nullptr, GetValue },
      { "setValue", nullptr, SetValue },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), NAPI_AUTO_LENGTH, cb, nullptr, 5, descriptors, &ctor));
    return ctor;
  }

  napi_value List::GetValue(napi_env env, napi_callback_info info) {
    return CommonGetIndexedValue(env, info, sass_list_get_length, sass_list_get_value);
  }

  napi_value List::SetValue(napi_env env, napi_callback_info info) {
    return CommonSetIndexedValue(env, info, sass_list_set_value);
  }

  napi_value List::GetSeparator(napi_env env, napi_callback_info info) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    bool v = sass_list_get_separator(unwrap(env, _this)->value) == SASS_COMMA;
    napi_value ret;
    CHECK_NAPI_RESULT(napi_get_boolean(env, v, &ret));
    return ret;
  }

  napi_value List::SetSeparator(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value arg;
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, &arg, &_this, nullptr));

    if (argc != 1) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Expected just one argument"));
      return nullptr;
    }

    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_typeof(env, arg, &t));

    if (t != napi_boolean) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied value should be a boolean"));
      return nullptr;
    }

    bool b;
    CHECK_NAPI_RESULT(napi_get_value_bool(env, arg, &b));

    sass_list_set_separator(unwrap(env, _this)->value, b ? SASS_COMMA : SASS_SPACE);
    return nullptr;
  }

  napi_value List::GetLength(napi_env env, napi_callback_info info) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    size_t s = sass_list_get_length(unwrap(env, _this)->value);
    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_double(env, (double)s, &ret));
    return ret;
  }
}
