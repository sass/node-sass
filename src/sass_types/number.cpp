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
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));

      if (t != napi_number) {
        return fail("First argument should be a number.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_double(env, raw_val[0], &value));

      if (raw_val.size() >= 2) {
        CHECK_NAPI_RESULT(napi_typeof(env, raw_val[1], &t));

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
        { "getValue", nullptr, GetValue },
        { "getUnit", nullptr, GetUnit },
        { "setValue", nullptr, SetValue },
        { "setUnit", nullptr, SetUnit },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), NAPI_AUTO_LENGTH, cb, nullptr, 4, descriptors, &ctor));
    return ctor;
  }

  napi_value Number::GetValue(napi_env env, napi_callback_info info) {
    return CommonGetNumber(env, info, sass_number_get_value);
  }

  napi_value Number::GetUnit(napi_env env, napi_callback_info info) {
    return CommonGetString(env, info, sass_number_get_unit);
  }

  napi_value Number::SetValue(napi_env env, napi_callback_info info) {
    return CommonSetNumber(env, info, sass_number_set_value);
  }

  napi_value Number::SetUnit(napi_env env, napi_callback_info info) {
    return CommonSetString(env, info, sass_number_set_unit);
  }
}
