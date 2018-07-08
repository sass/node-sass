#include <stdexcept>
#include "custom_function_bridge.h"
#include "sass_types/factory.h"
#include "sass_types/value.h"

Sass_Value* CustomFunctionBridge::post_process_return_value(napi_env env, napi_value v) const {
  SassTypes::Value *value = SassTypes::Factory::unwrap(env, v);
  if (value) {
    return value->get_sass_value();
  } else {
    return sass_make_error("A SassValue object was expected.");
  }
}

std::vector<napi_value> CustomFunctionBridge::pre_process_args(napi_env env, std::vector<void*> in) const {
  std::vector<napi_value> argv;

  for (void* value : in) {
    Sass_Value* x = static_cast<Sass_Value*>(value);
    SassTypes::Value* y = SassTypes::Factory::create(env, x);

    argv.push_back(y->get_js_object(env));
  }

  return argv;
}
