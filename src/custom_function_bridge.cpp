#include <nan.h>
#include <stdexcept>
#include "custom_function_bridge.h"
#include "sass_types/factory.h"

Sass_Value* CustomFunctionBridge::post_process_return_value(v8::Local<v8::Value> val) const {
  try {
    return SassTypes::Factory::unwrap(val)->get_sass_value();
  }
  catch (const std::invalid_argument& e) {
    return sass_make_error(e.what());
  }
}

std::vector<v8::Local<v8::Value>> CustomFunctionBridge::pre_process_args(std::vector<void*> in) const {
  std::vector<v8::Local<v8::Value>> argv = std::vector<v8::Local<v8::Value>>();

  for (void* value : in) {
    argv.push_back(SassTypes::Factory::create(static_cast<Sass_Value*>(value))->get_js_object());
  }

  return argv;
}
