#include "null.h"

namespace SassTypes
{
  napi_ref Null::constructor = nullptr;
  bool Null::constructor_locked = false;

  Null::Null() : js_object(nullptr) {}

  Null& Null::get_singleton() {
    static Null singleton_instance;
    return singleton_instance;
  }

  napi_value Null::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope;
    napi_value ctor;

    if (Null::constructor) {
      CHECK_NAPI_RESULT(napi_get_reference_value(env, Null::constructor, &ctor));
    } else {
      CHECK_NAPI_RESULT(napi_define_class(env, "SassNull", Null::New, nullptr, 0, nullptr, &ctor));
      CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &Null::constructor));

      Null& singleton = get_singleton();
      napi_value instance;
      CHECK_NAPI_RESULT(napi_new_instance(env, ctor, 0, nullptr, &instance));
      CHECK_NAPI_RESULT(napi_wrap(env, instance, &singleton, nullptr, nullptr));
      CHECK_NAPI_RESULT(napi_create_reference(env, instance, 1, &singleton.js_object));

      napi_propertyname nullName;
      CHECK_NAPI_RESULT(napi_property_name(env, "NULL", &nullName));
      CHECK_NAPI_RESULT(napi_set_property(env, ctor, nullName, instance));

      constructor_locked = true;
    }

    return scope.Escape(ctor);
  }

  Sass_Value* Null::get_sass_value() {
    return sass_make_null();
  }

  napi_value Null::get_js_object(napi_env env) {
    napi_value v;
    CHECK_NAPI_RESULT(napi_get_reference_value(env, this->js_object, &v));
    return v;
  }

  NAPI_METHOD(Null::New) {
    bool r;
    CHECK_NAPI_RESULT(napi_is_construct_call(env, info, &r));

    if (r) {
      if (constructor_locked) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, "Cannot instantiate SassNull"));
        return;
      }
    }
    else {
      napi_value obj = Null::get_singleton().get_js_object(env);
      CHECK_NAPI_RESULT(napi_set_return_value(env, info, obj));
    }
  }
}
