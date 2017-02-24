#ifndef CUSTOM_FUNCTION_BRIDGE_H
#define CUSTOM_FUNCTION_BRIDGE_H

#include <sass/values.h>
#include <sass/functions.h>
#include "callback_bridge.h"

class CustomFunctionBridge : public CallbackBridge<Sass_Value*> {
  public:
    CustomFunctionBridge(napi_env env, napi_value cb, bool is_sync) : CallbackBridge<Sass_Value*>(env, cb, is_sync) {}

  private:
    Sass_Value* post_process_return_value(napi_env, napi_value) const;
    std::vector<napi_value> pre_process_args(napi_env, std::vector<void*>) const;
};

#endif
