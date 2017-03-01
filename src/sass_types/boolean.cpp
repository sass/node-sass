#include "boolean.h"

namespace SassTypes
{
  napi_ref Boolean::constructor = nullptr;
  bool Boolean::constructor_locked = false;

  Boolean::Boolean(bool v) : value(v), js_object(nullptr) {}

  Boolean& Boolean::get_singleton(bool v) {
    static Boolean instance_false(false), instance_true(true);
    return v ? instance_true : instance_false;
  }

  napi_value Boolean::construct_and_wrap_instance(napi_env env, napi_value ctor, Boolean* b) {
    Napi::EscapableHandleScope scope;

    napi_value instance;
    CHECK_NAPI_RESULT(napi_new_instance(env, ctor, 0, nullptr, &instance));
    CHECK_NAPI_RESULT(napi_wrap(env, instance, b, nullptr, nullptr));
    CHECK_NAPI_RESULT(napi_create_reference(env, instance, 1, &(b->js_object)));

    return scope.Escape(instance);
  }

  napi_value Boolean::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope;
    napi_value ctor;

    if (Boolean::constructor) {
      CHECK_NAPI_RESULT(napi_get_reference_value(env, Boolean::constructor, &ctor));
    } else {
      napi_property_descriptor methods[] = {
        { "getValue", Boolean::GetValue },
      };
      
      CHECK_NAPI_RESULT(napi_define_class(env, "SassBoolean", Boolean::New, nullptr, 1, methods, &ctor));
      CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &Boolean::constructor));

      Boolean& falseSingleton = get_singleton(false);
      napi_value instance = construct_and_wrap_instance(env, ctor, &falseSingleton);

      napi_propertyname falseName;
      CHECK_NAPI_RESULT(napi_property_name(env, "FALSE", &falseName));
      CHECK_NAPI_RESULT(napi_set_property(env, ctor, falseName, instance));

      Boolean& trueSingleton = get_singleton(true);
      instance = construct_and_wrap_instance(env, ctor, &trueSingleton);

      napi_propertyname trueName;
      CHECK_NAPI_RESULT(napi_property_name(env, "TRUE", &trueName));
      CHECK_NAPI_RESULT(napi_set_property(env, ctor, trueName, instance));

      constructor_locked = true;
    }

    return scope.Escape(ctor);
  }

  Sass_Value* Boolean::get_sass_value() {
    return sass_make_boolean(value);
  }

  napi_value Boolean::get_js_object(napi_env env) {
    Napi::EscapableHandleScope scope;
    napi_value v;
    CHECK_NAPI_RESULT(napi_get_reference_value(env, this->js_object, &v));
    return scope.Escape(v);
  }

  NAPI_METHOD(Boolean::New) {
    bool r;
    CHECK_NAPI_RESULT(napi_is_construct_call(env, info, &r));

    if (r) {
      if (constructor_locked) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, "Cannot instantiate SassBoolean"));
        return;
      }
    } else {
      int argsLength;
      CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argsLength));

      if (argsLength != 1) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected one boolean argument"));
        return;
      }

      napi_value argv[1];
      CHECK_NAPI_RESULT(napi_get_cb_args(env, info, argv, 1));
      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv[0], &t));

      if (t != napi_boolean) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected one boolean argument"));
        return;
      }

      CHECK_NAPI_RESULT(napi_get_value_bool(env, argv[0], &r));
      napi_value obj = Boolean::get_singleton(r).get_js_object(env);
      CHECK_NAPI_RESULT(napi_set_return_value(env, info, obj));
    }
  }

  NAPI_METHOD(Boolean::GetValue) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    Boolean *out = static_cast<Boolean*>(Factory::unwrap(env, _this));
    if (out) {
      napi_value b;
      CHECK_NAPI_RESULT(napi_create_boolean(env, out->value, &b));
      CHECK_NAPI_RESULT(napi_set_return_value(env, info, b));
    }
  }
}
