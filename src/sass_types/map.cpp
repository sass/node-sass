#include "map.h"

namespace SassTypes
{
  Map::Map(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* Map::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    uint32_t length = 0;

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_typeof(env, raw_val[0], &t));

      if (t != napi_number) {
        return fail("First argument should be an integer.", out);
      }

      CHECK_NAPI_RESULT(napi_get_value_uint32(env, raw_val[0], &length));
    }

    return *out = sass_make_map(length);
  }

  napi_value Map::getConstructor(napi_env env, napi_callback cb) {
    napi_value ctor;
    napi_property_descriptor descriptors[] = {
      { "getLength", nullptr, GetLength },
      { "getKey", nullptr, GetKey },
      { "setKey", nullptr, SetKey },
      { "getValue", nullptr, GetValue },
      { "setValue", nullptr, SetValue },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), NAPI_AUTO_LENGTH, cb, nullptr, 5, descriptors, &ctor));
    return ctor;
  }

  napi_value Map::GetValue(napi_env env, napi_callback_info info) {
    return CommonGetIndexedValue(env, info, sass_map_get_length, sass_map_get_value);
  }

  napi_value Map::SetValue(napi_env env, napi_callback_info info) {
    return CommonSetIndexedValue(env, info, sass_map_set_value);
  }

  napi_value Map::GetKey(napi_env env, napi_callback_info info) {
    return CommonGetIndexedValue(env, info, sass_map_get_length, sass_map_get_key);
  }

  napi_value Map::SetKey(napi_env env, napi_callback_info info) {
    return CommonSetIndexedValue(env, info, sass_map_set_key);
  }

  napi_value Map::GetLength(napi_env env, napi_callback_info info) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    size_t s = sass_map_get_length(unwrap(env, _this)->value);
    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_double(env, (double)s, &ret));
    return ret;
  }
}
