#include <stdexcept>
#include "custom_function_bridge.h"
#include "sass_types/factory.h"
#include "sass_types/value.h"

Sass_Value* CustomFunctionBridge::post_process_return_value(napi_env env, napi_value v) const {
  SassTypes::Value *v_;

  if ((v_ = SassTypes::Factory::unwrap(env, v))) {
    return v_->get_sass_value();
  } else {
    return sass_make_error("A SassValue object was expected.");
  }
}

std::vector<napi_value> CustomFunctionBridge::pre_process_args(napi_env env, std::vector<void*> in) const {
  std::vector<napi_value> argv;

  for (void* value : in) {
    argv.push_back(SassTypes::Factory::create(env, static_cast<Sass_Value*>(value))->get_js_object(env));
  }

  return argv;
}
