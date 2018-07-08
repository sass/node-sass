#include "factory.h"
#include "value.h"
#include "number.h"
#include "string.h"
#include "color.h"
#include "boolean.h"
#include "list.h"
#include "map.h"
#include "null.h"
#include "error.h"
#include "../common.h"

namespace SassTypes
{
  SassTypes::Value* Factory::create(napi_env env, Sass_Value* v) {
    switch (sass_value_get_tag(v)) {
    case SASS_NUMBER:
      return new Number(env, v);

    case SASS_STRING:
      return new String(env, v);

    case SASS_COLOR:
      return new Color(env, v);

    case SASS_BOOLEAN:
      return &Boolean::get_singleton(sass_boolean_get_value(v));

    case SASS_LIST:
      return new List(env, v);

    case SASS_MAP:
      return new Map(env, v);

    case SASS_NULL:
      return &Null::get_singleton();

    case SASS_ERROR:
      return new Error(env, v);

    default:
      const char *msg = "Unknown type encountered.";
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, msg));
      return new Error(env, sass_make_error(msg));
    }
  }

  void Factory::initExports(napi_env env, napi_value target) {
    Napi::HandleScope scope(env);

    napi_value types;
    CHECK_NAPI_RESULT(napi_create_object(env, &types));

    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Number", Number::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "String", String::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Color", Color::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Boolean", Boolean::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "List", List::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Map", Map::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Null", Null::get_constructor(env)));
    CHECK_NAPI_RESULT(napi_set_named_property(env, types, "Error", Error::get_constructor(env)));

    CHECK_NAPI_RESULT(napi_set_named_property(env, target, "types", types));
  }

  Value* Factory::unwrap(napi_env env, napi_value obj) {
    void* wrapped;
    napi_status status = napi_unwrap(env, obj, &wrapped);
    if (status != napi_ok) {
      wrapped = nullptr;
    }
    return static_cast<Value*>(wrapped);
  }
}
