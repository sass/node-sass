#ifndef CALLBACK_BRIDGE_H
#define CALLBACK_BRIDGE_H

#include <vector>
#include <algorithm>
#include <uv.h>
#include <node_jsvmapi_types.h>
#include <node_api_helpers.h>
#include "common.h"

#define COMMA ,

template <typename T, typename L = void*>
class CallbackBridge {
  public:
    CallbackBridge(napi_env, napi_value, bool);
    virtual ~CallbackBridge();

    // Executes the callback
    T operator()(std::vector<void*>);

    // Needed for napi_wrap
    napi_value NewInstance(napi_env env);

  protected:
    // We will expose a bridge object to the JS callback that wraps this instance so we don't loose context.
    // This is the V8 constructor for such objects.
    static napi_ref get_wrapper_constructor(napi_env env);
    static void async_gone(uv_handle_t *handle);
    static void New(napi_env env, napi_callback_info info);
    static void ReturnCallback(napi_env env, napi_callback_info info);
    static napi_ref wrapper_constructor;
    napi_ref wrapper;

    // The callback that will get called in the main thread after the worker thread used for the sass
    // compilation step makes a call to uv_async_send()
    static void dispatched_async_uv_callback(uv_async_t*);

    // The V8 values sent to our ReturnCallback must be read on the main thread not the sass worker thread.
    // This gives a chance to specialized subclasses to transform those values into whatever makes sense to
    // sass before we resume the worker thread.
    virtual T post_process_return_value(napi_env, napi_value) const = 0;

    virtual std::vector<napi_value> pre_process_args(napi_env, std::vector<L>) const = 0;

    napi_env e;
    napi_value callback;
    bool is_sync;

    uv_mutex_t cv_mutex;
    uv_cond_t condition_variable;
    uv_async_t *async;
    std::vector<L> argv;
    bool has_returned;
    T return_value;
};

template <typename T, typename L>
napi_ref CallbackBridge<T, L>::wrapper_constructor = nullptr;

template <typename T, typename L>
void CallbackBridge_Destructor(void* obj) {
  CallbackBridge<T, L>* bridge = static_cast<CallbackBridge<T, L>*>(obj);
  delete bridge;
}

template <typename T, typename L>
napi_value CallbackBridge<T, L>::NewInstance(napi_env env) {
  Napi::EscapableHandleScope scope(env);

  napi_value instance;
  napi_value constructorHandle;
  CHECK_NAPI_RESULT(napi_get_reference_value(env, get_wrapper_constructor(env), &constructorHandle));
  CHECK_NAPI_RESULT(napi_new_instance(env, constructorHandle, 0, nullptr, &instance));

  return scope.Escape(instance);
}

template <typename T, typename L>
void CallbackBridge<T, L>::New(napi_env env, napi_callback_info info) {
  napi_value _this;
  CHECK_NAPI_RESULT(napi_get_cb_this(env, info, &_this));
  CHECK_NAPI_RESULT(napi_set_return_value(env, info, _this));
}

template <typename T, typename L>
CallbackBridge<T, L>::CallbackBridge(napi_env env, napi_value callback, bool is_sync) : e(env), callback(callback), is_sync(is_sync) {
  /* 
   * This is invoked from the main JavaScript thread.
   * V8 context is available.
   */
  uv_mutex_init(&this->cv_mutex);
  uv_cond_init(&this->condition_variable);
  if (!is_sync) {
    this->async = new uv_async_t;
    this->async->data = (void*) this;
    uv_async_init(uv_default_loop(), this->async, (uv_async_cb) dispatched_async_uv_callback);
  }
  
  napi_value instance = CallbackBridge::NewInstance(env);
  CHECK_NAPI_RESULT(napi_wrap(env, instance, this, CallbackBridge_Destructor<T,L>, nullptr));
  CHECK_NAPI_RESULT(napi_create_reference(env, instance, 1, &this->wrapper));
}

template <typename T, typename L>
CallbackBridge<T, L>::~CallbackBridge() {
  CHECK_NAPI_RESULT(napi_reference_release(e, this->wrapper, nullptr));

  uv_cond_destroy(&this->condition_variable);
  uv_mutex_destroy(&this->cv_mutex);

  if (!is_sync) {
    uv_close((uv_handle_t*)this->async, &async_gone);
  }
}

template <typename T, typename L>
T CallbackBridge<T, L>::operator()(std::vector<void*> argv) {
  // argv.push_back(wrapper);
  if (this->is_sync) {
    /* 
     * This is invoked from the main JavaScript thread.
     * V8 context is available.
     *
     * Establish Local<> scope for all functions
     * from types invoked by pre_process_args() and
     * post_process_args().
     */
    Napi::HandleScope scope;

    std::vector<napi_value> argv_v8 = pre_process_args(this->e, argv);

    bool isPending;
    CHECK_NAPI_RESULT(napi_is_exception_pending(this->e, &isPending));

    if (isPending) {
      CHECK_NAPI_RESULT(napi_throw_error(this->e, "Error processing arguments"));
      // This should be a FatalException but we still have to return something, this value might be uninitialized
      return this->return_value;
    }

    napi_value _this;
    CHECK_NAPI_RESULT(napi_get_persistent_value(this->e, this->wrapper, &_this));
    argv_v8.push_back(_this);

    napi_value result;
    // TODO: Is receiver set correctly ?
    CHECK_NAPI_RESULT(napi_make_callback(this->e, _this, this->callback, argv_v8.size(), &argv_v8[0], &result));

    return this->post_process_return_value(this->e, result);
  } else {
    /* 
     * This is invoked from the worker thread.
     * No V8 context and functions available.
     * Just wait for response from asynchronously
     * scheduled JavaScript code
     *
     * XXX Issue #1048: We block here even if the
     *     event loop stops and the callback
     *     would never be executed.
     * XXX Issue #857: By waiting here we occupy
     *     one of the threads taken from the
     *     uv threadpool. Might deadlock if
     *     async I/O executed from JavaScript callbacks.
     */
    this->argv = argv;

    uv_mutex_lock(&this->cv_mutex);
    this->has_returned = false;
    uv_async_send(this->async);
    while (!this->has_returned) {
      uv_cond_wait(&this->condition_variable, &this->cv_mutex);
    }
    uv_mutex_unlock(&this->cv_mutex);
    return this->return_value;
  }
}

template <typename T, typename L>
void CallbackBridge<T, L>::dispatched_async_uv_callback(uv_async_t *req) {
  CallbackBridge* bridge = static_cast<CallbackBridge*>(req->data);

  /* 
   * Function scheduled via uv_async mechanism, therefore
   * it is invoked from the main JavaScript thread.
   * V8 context is available.
   *
   * Establish Local<> scope for all functions
   * from types invoked by pre_process_args() and
   * post_process_args().
   */
  Napi::HandleScope scope;
  std::vector<napi_value> argv_v8 = bridge->pre_process_args(bridge->e, bridge->argv);
  bool isPending;

  CHECK_NAPI_RESULT(napi_is_exception_pending(bridge->e, &isPending));
  if (isPending) {
      CHECK_NAPI_RESULT(napi_throw_error(bridge->e, "Error processing arguments"));
      // This should be a FatalException 
      return;
  }

  napi_value _this;
  CHECK_NAPI_RESULT(napi_get_reference_value(bridge->e, bridge->wrapper, &_this));
  argv_v8.push_back(_this);

  napi_value result;
  // TODO: Is receiver set correctly ?
  CHECK_NAPI_RESULT(napi_make_callback(bridge->e, _this, bridge->callback, argv_v8.size(), &argv_v8[0], &result));
  CHECK_NAPI_RESULT(napi_is_exception_pending(bridge->e, &isPending));
  if (isPending) {
      CHECK_NAPI_RESULT(napi_throw_error(bridge->e, "Error thrown in callback"));
      // This should be a FatalException 
      return;
  }
}

template <typename T, typename L>
void CallbackBridge<T, L>::ReturnCallback(napi_env env, napi_callback_info info) {
  napi_value args[1];
  CHECK_NAPI_RESULT(napi_get_cb_args(env, info, args, 1));
  void* unwrapped;
  CHECK_NAPI_RESULT(napi_unwrap(env, args[0], &unwrapped));
  CallbackBridge* bridge = static_cast<CallbackBridge*>(unwrapped);

  /* 
   * Callback function invoked by the user code.
   * It is invoked from the main JavaScript thread.
   * V8 context is available.
   *
   * Implicit Local<> handle scope created by NAN_METHOD(.)
   */

  bridge->return_value = bridge->post_process_return_value(env, args[0]);

  {
    uv_mutex_lock(&bridge->cv_mutex);
    bridge->has_returned = true;
    uv_mutex_unlock(&bridge->cv_mutex);
  }

  uv_cond_broadcast(&bridge->condition_variable);
  
  bool isPending;
  CHECK_NAPI_RESULT(napi_is_exception_pending(env, &isPending));

  if (isPending) {
      // TODO: Call node::FatalException or add napi_fatal_exception ?
      assert(false);
  }
}

template <typename T, typename L>
napi_ref CallbackBridge<T, L>::get_wrapper_constructor(napi_env env) {
  // TODO: cache wrapper_constructor

  napi_property_descriptor methods[] = {
    { "success", CallbackBridge::ReturnCallback },
  };

  napi_value ctor;
  CHECK_NAPI_RESULT(napi_define_class(env, "CallbackBridge", CallbackBridge::New, nullptr, 1, methods, &ctor));
  CHECK_NAPI_RESULT(napi_create_reference(env, ctor, 1, &wrapper_constructor));

  return wrapper_constructor;
}

template <typename T, typename L>
void CallbackBridge<T, L>::async_gone(uv_handle_t *handle) {
  delete (uv_async_t *)handle;
}

#endif
