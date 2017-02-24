#include "map.h"

namespace SassTypes
{
  Map::Map(napi_env env, Sass_Value* v) : SassValueWrapper(env, v) {}

  Sass_Value* Map::construct(napi_env env, const std::vector<napi_value> raw_val, Sass_Value **out) {
    uint32_t length = 0;

    if (raw_val.size() >= 1) {
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, raw_val[0], &t));

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
      { "getLength", GetLength },
      { "getKey", GetKey },
      { "setKey", SetKey },
      { "getValue", GetValue },
      { "setValue", SetValue },
    };

    CHECK_NAPI_RESULT(napi_define_class(env, get_constructor_name(), cb, nullptr, 5, descriptors, &ctor));
    return ctor;
  }

  NAPI_METHOD(Map::GetValue) {
    CommonGetIndexedValue(env, info, sass_map_get_length, sass_map_get_value);
  }

  NAPI_METHOD(Map::SetValue) {
    CommonSetIndexedValue(env, info, sass_map_set_value);
  }

  NAPI_METHOD(Map::GetKey) {
    CommonGetIndexedValue(env, info, sass_map_get_length, sass_map_get_key);
  }

  NAPI_METHOD(Map::SetKey) {
    CommonSetIndexedValue(env, info, sass_map_set_key);
  }

  NAPI_METHOD(Map::GetLength) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    size_t s = sass_map_get_length(unwrap(env, _this)->value);
    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_number(env, (double)s, &ret));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, ret));
  }
}
