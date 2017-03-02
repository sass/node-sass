#ifndef SASS_TYPES_SASS_VALUE_WRAPPER_H
#define SASS_TYPES_SASS_VALUE_WRAPPER_H

#include <stdexcept>
#include <vector>
#include "value.h"
#include "factory.h"
#include <node_api_helpers.h>
#include <node_jsvmapi_types.h>

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
      static NAPI_METHOD(New);
      static Sass_Value *fail(const char *, Sass_Value **);

    protected:
      Sass_Value* value;
      static T* unwrap(napi_env, napi_value);

      static void CommonGetNumber(napi_env env, napi_callback_info info, double(fnc)(const Sass_Value*));
      static void CommonSetNumber(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, double));

      static void CommonGetString(napi_env env, napi_callback_info info, const char*(fnc)(const Sass_Value*));
      static void CommonSetString(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, char*));

      static void CommonGetIndexedValue(napi_env env, napi_callback_info info, size_t(lenfnc)(const Sass_Value*), Sass_Value*(getfnc)(const Sass_Value*, size_t));
      static void CommonSetIndexedValue(napi_env env, napi_callback_info info, void(setfnc)(Sass_Value*, size_t, Sass_Value*));

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
  void SassValueWrapper<T>::CommonGetNumber(napi_env env, napi_callback_info info, double(fnc)(const Sass_Value*)) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    double d = fnc(unwrap(env, _this)->value);

    napi_value ret;
    CHECK_NAPI_RESULT(napi_create_number(env, d, &ret));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, ret));
  }

  template <class T>
  void SassValueWrapper<T>::CommonSetNumber(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, double)) {
    int argLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argLength));

    if (argLength != 1) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected just one argument"));
      return;
    }

    napi_value argv;
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &argv, 1));
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv, &t));

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a number"));
      return;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    double d;
    CHECK_NAPI_RESULT(napi_get_value_double(env, argv, &d));

    fnc(unwrap(env, _this)->value, d);
  }

  template <class T>
  void SassValueWrapper<T>::CommonGetString(napi_env env, napi_callback_info info, const char*(fnc)(const Sass_Value*)) {
    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    const char* v = fnc(unwrap(env, _this)->value);
    int len = (int)strlen(v);

    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, v, len, &str));
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, str));
  }

  template <class T>
  void SassValueWrapper<T>::CommonSetString(napi_env env, napi_callback_info info, void(fnc)(Sass_Value*, char*)) {
    int argLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argLength));

    if (argLength != 1) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected just one argument"));
      return;
    }

    napi_value argv;
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &argv, 1));
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv, &t));

    if (t != napi_string) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a string"));
      return;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));

    char* s = create_string(env, argv);

    fnc(unwrap(env, _this)->value, s);
  }

  template <class T>
  void SassValueWrapper<T>::CommonGetIndexedValue(napi_env env, napi_callback_info info, size_t(lenfnc)(const Sass_Value*), Sass_Value*(getfnc)(const Sass_Value*,size_t)) {
    int argLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argLength));

    if (argLength != 1) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected just one argument"));
      return;
    }

    napi_value argv;
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &argv, 1));
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv, &t));

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied index should be an integer"));
      return;
    }

    napi_value _this;
    uint32_t index;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));
    CHECK_NAPI_RESULT(napi_get_value_uint32(env, argv, &index));

    Sass_Value* collection = unwrap(env, _this)->value;

    if (index >= lenfnc(collection)) {
      CHECK_NAPI_RESULT(napi_throw_range_error(env, "Out of bound index"));
      return;
    }

    napi_value ret = Factory::create(env, getfnc(collection, index))->get_js_object(env);
    CHECK_NAPI_RESULT(napi_set_return_value(env, info, ret));
  }

  template <class T>
  void SassValueWrapper<T>::CommonSetIndexedValue(napi_env env, napi_callback_info info, void(setfnc)(Sass_Value*, size_t, Sass_Value*)) {
    int argLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argLength));

    if (argLength != 2) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Expected two arguments"));
      return;
    }

    napi_value argv[2];
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, argv, 2));
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv[0], &t));

    if (t != napi_number) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied index should be an integer"));
      return;
    }

    CHECK_NAPI_RESULT(napi_get_type_of_value(env, argv[1], &t));

    if (t != napi_object) {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "Supplied value should be a SassValue object"));
      return;
    }

    napi_value _this;
    uint32_t v;
    CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));
    CHECK_NAPI_RESULT(napi_get_value_uint32(env, argv[0], &v));

    Value* sass_value = Factory::unwrap(env, argv[1]);
    if (sass_value) {
        setfnc(unwrap(env, _this)->value, v, sass_value->get_sass_value());
    }
    else {
      CHECK_NAPI_RESULT(napi_throw_type_error(env, "A SassValue is expected as the list item"));
    }
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
      CHECK_NAPI_RESULT(napi_unwrap(env, wrapper, &wrapped));
      delete static_cast<T*>(wrapped);
      CHECK_NAPI_RESULT(napi_wrap(env, wrapper, this, nullptr, nullptr));
      CHECK_NAPI_RESULT(napi_create_reference(env, wrapper, 1, &this->js_object));
    }

    napi_value v;
    CHECK_NAPI_RESULT(napi_get_reference_value(env, this->js_object, &v));
    return v;
  }

  template <class T>
  napi_value SassValueWrapper<T>::get_constructor(napi_env env) {
    Napi::EscapableHandleScope scope;

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
  NAPI_METHOD(SassValueWrapper<T>::New) {
    int argsLength;
    CHECK_NAPI_RESULT(napi_get_cb_args_length(env, info, &argsLength));
    std::vector<napi_value> localArgs(argsLength);
    napi_value* argv = (napi_value*)malloc(sizeof(napi_value)*argsLength);
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, argv, argsLength));

    for (auto i = 0; i < argsLength; ++i) {
      localArgs[i] = argv[i];
    }
    free(argv);

    bool r;
    CHECK_NAPI_RESULT(napi_is_construct_call(env, info, &r));

    if (r) {
      Sass_Value* value;
      if (T::construct(env, localArgs, &value) != NULL) {
        T* obj = new T(env, value);
        sass_delete_value(value);

        napi_value _this;
        CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));
        CHECK_NAPI_RESULT(napi_wrap(env, _this, obj, nullptr, nullptr));
        CHECK_NAPI_RESULT(napi_create_reference(env, _this, 1, &obj->js_object));
      } else {
        CHECK_NAPI_RESULT(napi_throw_error(env, sass_error_get_message(value)));
        return;
      }
    } else {
      napi_value ctor = T::get_constructor(env);
      napi_value instance;
      napi_status status = napi_new_instance(env, ctor, argsLength, &localArgs[0], &instance);
      if (status == napi_ok) {
        CHECK_NAPI_RESULT(napi_set_return_value(env, info, instance));
      }
      else {
        napi_value undef;
        CHECK_NAPI_RESULT(napi_get_undefined(env, &undef));
        CHECK_NAPI_RESULT(napi_set_return_value(env, info, undef));
      }
    }
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
