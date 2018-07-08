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

  napi_value Null::construct_and_wrap_instance(napi_env env, napi_value ctor, Null* n) {
      Napi::EscapableHandleScope scope(env);

      napi_value instance;
      CHECK_NAPI_RESULT(napi_new_instance(env, ctor, 0, nullptr, &instance));
      CHECK_NAPI_RESULT(napi_wrap(env, instance, n, nullptr, nullptr, nullptr));
      CHECK_NAPI_RESULT(napi_create_reference(env, instance, 1, &(n->js_object)));

      return scope.Escape(instance);
  }

  napi_value Null::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope(env);
    napi_value ctor;

    if (Null::constructor) {
      CHECK_NAPI_RESULT(napi_get_reference_value(env, Null::constructor, &ctor));
    } else {
      CHECK_NAPI_RESULT(napi_define_class(env, "SassNull", NAPI_AUTO_LENGTH, Null::New, nullptr, 0, nullptr, &ctor));
      CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &Null::constructor));

      Null& singleton = get_singleton();
      napi_value instance = construct_and_wrap_instance(env, ctor, &singleton);

      CHECK_NAPI_RESULT(napi_set_named_property(env, ctor, "NULL", instance));

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

  napi_value Null::New(napi_env env, napi_callback_info info) {
    napi_value t;
    CHECK_NAPI_RESULT(napi_get_new_target(env, info, &t));
    bool r = (t != nullptr);

    if (r) {
      if (constructor_locked) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Cannot instantiate SassNull"));
      }
    }
    else {
      napi_value obj = Null::get_singleton().get_js_object(env);
      return obj;
    }
    return nullptr;
  }
}
