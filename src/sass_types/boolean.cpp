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
    Napi::EscapableHandleScope scope(env);

    napi_value instance;
    CHECK_NAPI_RESULT(napi_new_instance(env, ctor, 0, nullptr, &instance));
    CHECK_NAPI_RESULT(napi_wrap(env, instance, b, nullptr, nullptr, nullptr));
    CHECK_NAPI_RESULT(napi_create_reference(env, instance, 1, &(b->js_object)));

    return scope.Escape(instance);
  }

  napi_value Boolean::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope(env);
    napi_value ctor;

    if (Boolean::constructor) {
      CHECK_NAPI_RESULT(napi_get_reference_value(env, Boolean::constructor, &ctor));
    } else {
      napi_property_descriptor methods[] = {
        { "getValue", nullptr, Boolean::GetValue },
      };

      CHECK_NAPI_RESULT(napi_define_class(env, "SassBoolean", NAPI_AUTO_LENGTH, Boolean::New, nullptr, 1, methods, &ctor));
      CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &Boolean::constructor));

      Boolean& falseSingleton = get_singleton(false);
      napi_value instance = construct_and_wrap_instance(env, ctor, &falseSingleton);
      CHECK_NAPI_RESULT(napi_set_named_property(env, ctor, "FALSE", instance));

      Boolean& trueSingleton = get_singleton(true);
      instance = construct_and_wrap_instance(env, ctor, &trueSingleton);
      CHECK_NAPI_RESULT(napi_set_named_property(env, ctor, "TRUE", instance));

      constructor_locked = true;
    }

    return scope.Escape(ctor);
  }

  Sass_Value* Boolean::get_sass_value() {
    return sass_make_boolean(value);
  }

  napi_value Boolean::get_js_object(napi_env env) {
    Napi::EscapableHandleScope scope(env);
    napi_value v;
    CHECK_NAPI_RESULT(napi_get_reference_value(env, this->js_object, &v));
    return scope.Escape(v);
  }

  napi_value Boolean::New(napi_env env, napi_callback_info info) {
    napi_value t;
    CHECK_NAPI_RESULT(napi_get_new_target(env, info, &t));
    bool r = (t != nullptr);

    if (r) {
      if (constructor_locked) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Cannot instantiate SassBoolean"));
        return nullptr;
      }
    } else {
      size_t argc = 1;
      napi_value argv[1];
      CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

      if (argc != 1) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Expected one boolean argument"));
        return nullptr;
      }

      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_typeof(env, argv[0], &t));

      if (t != napi_boolean) {
        CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Expected one boolean argument"));
        return nullptr;
      }

      CHECK_NAPI_RESULT(napi_get_value_bool(env, argv[0], &r));
      napi_value obj = Boolean::get_singleton(r).get_js_object(env);
      return obj;
    }
    return nullptr;
  }

  napi_value Boolean::GetValue(napi_env env, napi_callback_info info) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    Boolean *out = static_cast<Boolean*>(Factory::unwrap(env, _this));
    if (out) {
      napi_value b;
      CHECK_NAPI_RESULT(napi_get_boolean(env, out->value, &b));
      return b;
    }
    return nullptr;
  }
}
