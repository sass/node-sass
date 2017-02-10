#include <nan.h>
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

namespace SassTypes
{
  SassTypes::Value* Factory::create(Sass_Value* v) {
    switch (sass_value_get_tag(v)) {
    case SASS_NUMBER:
      return new Number(v);

    case SASS_STRING:
      return new String(v);

    case SASS_COLOR:
      return new Color(v);

    case SASS_BOOLEAN:
      return &Boolean::get_singleton(sass_boolean_get_value(v));

    case SASS_LIST:
      return new List(v);

    case SASS_MAP:
      return new Map(v);

    case SASS_NULL:
      return &Null::get_singleton();

    case SASS_ERROR:
      return new Error(v);

    default:
      const char *msg = "Unknown type encountered.";
      Nan::ThrowTypeError(msg);
      return new Error(sass_make_error(msg));
    }
  }

  void Factory::initExports(napi_value target) {
    napi_env e;
    CHECK_NAPI_RESULT(napi_get_current_env(&e));
    napi_handle_scope scope;
    CHECK_NAPI_RESULT(napi_open_handle_scope(e, &scope));

    napi_value types;
    CHECK_NAPI_RESULT(napi_create_object(e, &types));
    
    napi_propertyname nameNumber;
    CHECK_NAPI_RESULT(napi_property_name(e, "Number", &nameNumber));
    napi_propertyname nameString;
    CHECK_NAPI_RESULT(napi_property_name(e, "String", &nameString));
    napi_propertyname nameColor;
    CHECK_NAPI_RESULT(napi_property_name(e, "Color", &nameColor));
    napi_propertyname nameBoolean;
    CHECK_NAPI_RESULT(napi_property_name(e, "Boolean", &nameBoolean));
    napi_propertyname nameList;
    CHECK_NAPI_RESULT(napi_property_name(e, "List", &nameList));
    napi_propertyname nameMap;
    CHECK_NAPI_RESULT(napi_property_name(e, "Map", &nameMap));
    napi_propertyname nameNull;
    CHECK_NAPI_RESULT(napi_property_name(e, "Null", &nameNull));
    napi_propertyname nameError;
    CHECK_NAPI_RESULT(napi_property_name(e, "Error", &nameError));
    napi_propertyname nameTypes;
    CHECK_NAPI_RESULT(napi_property_name(e, "types", &nameTypes));

    CHECK_NAPI_RESULT(napi_set_property(e, types, nameNumber, Number::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameString, String::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameColor, Color::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameBoolean, Boolean::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameList, List::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameMap, Map::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameNull, Null::get_constructor()));
    CHECK_NAPI_RESULT(napi_set_property(e, types, nameError, Error::get_constructor()));

    CHECK_NAPI_RESULT(napi_set_property(e, target, nameTypes, types));
  }

  Value* Factory::unwrap(v8::Local<v8::Value> obj) {
    // Todo: non-SassValue objects could easily fall under that condition, need to be more specific.
    if (!obj->IsObject() || obj.As<v8::Object>()->InternalFieldCount() != 1) {
      return NULL;
    }

    return static_cast<Value*>(Nan::GetInternalFieldPointer(obj.As<v8::Object>(), 0));
  }
}
