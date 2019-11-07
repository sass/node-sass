#ifndef SASS_TYPES_SASS_VALUE_WRAPPER_H
#define SASS_TYPES_SASS_VALUE_WRAPPER_H

#include <stdexcept>
#include <vector>
#include "value.h"
#include "factory.h"
#include "../create_string.h"
#include <napi.h>

namespace SassTypes
{
  // Include this in any SassTypes::Value subclasses to handle all the heavy lifting of constructing JS
  // objects and wrapping sass values inside them
  template <class T>
  class SassValueWrapper : public SassTypes::Value {
    public:
      static char const* get_constructor_name() { return "SassValue"; }

      SassValueWrapper(napi_env, Sass_Value*);
      virtual ~SassValueWrapper();

      Sass_Value* get_sass_value();
      napi_value get_js_object(napi_env);

      static napi_value get_constructor(napi_env);
      static napi_value New(napi_env env, napi_callback_info info);
      static Sass_Value *fail(const char *, Sass_Value **);

    protected:
      Sass_Value* value;
      static T* unwrap(napi_env, napi_value);

      static napi_value CommonGetNumber(napi_env env, napi_callback_info info, double(fnc)(const Sass_Value*));
      static napi_value CommonSetNumber(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, double));

      static napi_value CommonGetString(napi_env env, napi_callback_info info, const char*(fnc)(const Sass_Value*));
      static napi_value CommonSetString(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, char*));

      static napi_value CommonGetIndexedValue(napi_env env, napi_callback_info info, size_t(lenfnc)(const Sass_Value*), Sass_Value*(getfnc)(const Sass_Value*, size_t));
      static napi_value CommonSetIndexedValue(napi_env env, napi_callback_info info, void(setfnc)(Sass_Value*, size_t, Sass_Value*));

    private:
      static napi_ref constructor;
      napi_ref js_object;
      napi_env e;
  };

  template <class T>
  napi_ref SassValueWrapper<T>::constructor = nullptr;

  template <class T>
  SassValueWrapper<T>::SassValueWrapper(napi_env env, Sass_Value* v) {
    this->value = sass_clone_value(v);
    this->e = env;
    this->js_object = nullptr;
  }

  template <class T>
  SassValueWrapper<T>::~SassValueWrapper() {
    CHECK_NAPI_RESULT(napi_delete_reference(this->e, this->js_object));
    sass_delete_value(this->value);
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonGetNumber(napi_env env, napi_callback_info info, double(fnc)(const Sass_Value*)) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    double d = fnc(unwrap(env, _this)->value);

    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_double(env, d, &ret));
    return ret;
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonSetNumber(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, double)) {
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

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied value should be a number"));
      return nullptr;
    }

    double d;
    CHECK_NAPI_RESULT(napi_get_value_double(env, arg, &d));

    fnc(unwrap(env, _this)->value, d);
    return nullptr;
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonGetString(napi_env env, napi_callback_info info, const char*(fnc)(const Sass_Value*)) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, nullptr, nullptr, &_this, nullptr));

    const char* v = fnc(unwrap(env, _this)->value);
    int len = (int)strlen(v);

    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, v, len, &str));
    return str;
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonSetString(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, char*)) {
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

    if (t != napi_string) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied value should be a string"));
      return nullptr;
    }

    char* s = create_string(env, arg);

    fnc(unwrap(env, _this)->value, s);
    return nullptr;
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonGetIndexedValue(napi_env env, napi_callback_info info, size_t(lenfnc)(const Sass_Value*), Sass_Value*(getfnc)(const Sass_Value*,size_t)) {
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

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied index should be an integer"));
      return nullptr;
    }

    uint32_t index;
    CHECK_NAPI_RESULT(napi_get_value_uint32(env, arg, &index));

    Sass_Value* collection = unwrap(env, _this)->value;

    if (index >= lenfnc(collection)) {
      CHECK_NAPI_RESULT(napi_throw_range_error(env, nullptr, "Out of bound index"));
      return nullptr;
    }

    napi_value ret = Factory::create(env, getfnc(collection, index))->get_js_object(env);
    return ret;
  }

  template <class T>
  napi_value SassValueWrapper<T>::CommonSetIndexedValue(napi_env env, napi_callback_info info, void(setfnc)(Sass_Value*, size_t, Sass_Value*)) {
    size_t argc = 2;
    napi_value argv[2];
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, argv, &_this, nullptr));

    if (argc != 2) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Expected two arguments"));
      return nullptr;
    }

    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_typeof(env, argv[0], &t));

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied index should be an integer"));
      return nullptr;
    }

    CHECK_NAPI_RESULT(napi_typeof(env, argv[1], &t));

    if (t != napi_object) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "Supplied value should be a SassValue object"));
      return nullptr;
    }

    uint32_t v;
    CHECK_NAPI_RESULT(napi_get_value_uint32(env, argv[0], &v));

    Value* sass_value = Factory::unwrap(env, argv[1]);
    if (sass_value) {
        setfnc(unwrap(env, _this)->value, v, sass_value->get_sass_value());
    }
    else {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "A SassValue is expected"));
    }
    return nullptr;
  }

  template <class T>
  Sass_Value* SassValueWrapper<T>::get_sass_value() {
    return sass_clone_value(this->value);
  }

  template <class T>
  napi_value SassValueWrapper<T>::get_js_object(napi_env env) {
    if (this->js_object == nullptr) {
      napi_value wrapper;
      napi_value ctor = T::get_constructor(env);
      CHECK_NAPI_RESULT(napi_new_instance(env, ctor, 0, nullptr, &wrapper));
      void* wrapped;
      CHECK_NAPI_RESULT(napi_remove_wrap(env, wrapper, &wrapped));
      delete static_cast<T*>(wrapped);
      CHECK_NAPI_RESULT(napi_wrap(env, wrapper, this, nullptr, nullptr, nullptr));
      CHECK_NAPI_RESULT(napi_create_reference(env, wrapper, 1, &this->js_object));
    }

    napi_value v;
    CHECK_NAPI_RESULT(napi_get_reference_value(env, this->js_object, &v));
    return v;
  }

  template <class T>
  napi_value SassValueWrapper<T>::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope(env);

    napi_value ctor;
    if (!constructor) {
      ctor = T::getConstructor(env, New);
      CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &constructor));
    }
    else {
      CHECK_NAPI_RESULT(napi_get_reference_value(env, constructor, &ctor));
    }

    return scope.Escape(ctor);
  }

  template <class T>
  napi_value SassValueWrapper<T>::New(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, nullptr, &_this, nullptr));
    std::vector<napi_value> argv(argc);
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, argv.data(), nullptr, nullptr));

    napi_value t;
    CHECK_NAPI_RESULT(napi_get_new_target(env, info, &t));
    bool r = (t != nullptr);

    if (r) {
      Sass_Value* value;
      if (T::construct(env, argv, &value) != NULL) {
        T* obj = new T(env, value);
        sass_delete_value(value);

        CHECK_NAPI_RESULT(napi_wrap(env, _this, obj, nullptr, nullptr, nullptr));
        CHECK_NAPI_RESULT(napi_create_reference(env, _this, 1, &obj->js_object));
        return _this;
      } else {
        CHECK_NAPI_RESULT(napi_throw_error(env, nullptr, sass_error_get_message(value)));
      }
    } else {
      napi_value ctor = T::get_constructor(env);
      napi_value instance;
      napi_status status = napi_new_instance(env, ctor, argc, argv.data(), &instance);
      if (status == napi_ok) {
        return instance;
      }
    }
    return nullptr;
  }

  template <class T>
  T* SassValueWrapper<T>::unwrap(napi_env env, napi_value obj) {
    /* This maybe NULL */
    return static_cast<T*>(Factory::unwrap(env, obj));
  }

  template <class T>
  Sass_Value *SassValueWrapper<T>::fail(const char *reason, Sass_Value **out) {
    *out = sass_make_error(reason);
    return NULL;
  }
}


#endif
