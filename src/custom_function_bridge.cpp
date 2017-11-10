#include <nan.h>
#include <stdexcept>
#include "custom_function_bridge.h"
#include "sass_types/factory.h"
#include "sass_types/value.h"
#include "debug.h"

Sass_Value* CustomFunctionBridge::post_process_return_value(v8::Local<v8::Value> val) const {
  SassTypes::Value *v_;
  if ((v_ = SassTypes::Factory::unwrap(val))) {
    TRACEINST(&val) << " CustomFunctionBridge: unwrapping custom function return value...";
    return v_->get_sass_value();
  } else {
    return sass_make_error("A SassValue object was expected.");
  }
}

std::vector<v8::Local<v8::Value>> CustomFunctionBridge::pre_process_args(std::vector<void*> in) const {
  std::vector<v8::Local<v8::Value>> argv = std::vector<v8::Local<v8::Value>>();

  for (void* value : in) {
    TRACEINST(&value) << " CustomFunctionBridge: wrapping custom function parameters...";
    argv.push_back(SassTypes::Factory::create(static_cast<Sass_Value*>(value))->get_js_object());
  }

  return argv;
}
