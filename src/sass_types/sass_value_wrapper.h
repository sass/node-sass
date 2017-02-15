#ifndef SASS_TYPES_SASS_VALUE_WRAPPER_H
#define SASS_TYPES_SASS_VALUE_WRAPPER_H

#include <stdexcept>
#include <vector>
#include <nan.h>
#include "value.h"
#include "factory.h"
#include <node_api_helpers.h>

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
    int unused;
    CHECK_NAPI_RESULT(napi_reference_release(this->e, this->js_object, &unused));
    sass_delete_value(this->value);
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
    napi_value argv[argsLength];
    CHECK_NAPI_RESULT(napi_get_cb_args(env, info, argv, argsLength));
    
    for (auto i = 0; i < argsLength; ++i) {
      localArgs[i] = argv[i];
    }
    
    bool r;
    CHECK_NAPI_RESULT(napi_is_construct_call(env, info, &r));

    if (r) {
      Sass_Value* value;
      if (T::construct(env, localArgs, &value) != NULL) {
        T* obj = new T(value);
        sass_delete_value(value);

        napi_value _this;
        CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));
        CHECK_NAPI_RESULT(napi_wrap(env, _this, obj, nullptr, nullptr));
        CHECK_NAPI_RESULT(napi_create_reference(env, _this, &obj->js_object));
      } else {
        CHECK_NAPI_RESULT(napi_throw_error(sass_error_get_message(value)));
        return;
      }
    } else {
      napi_value ctor = T::get_constructor(env);
      napi_value instance;
      CHECK_NAPI_RESULT(napi_new_instance(env, ctor, argsLength, &localArgs[0], &instance));
      CHECK_NAPI_RESULT(napi_set_return_value(env, info, instance));

      // TODO: If new instance fails, return undefined
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
