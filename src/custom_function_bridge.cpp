#include <nan.h>
#include <stdexcept>
#include "custom_function_bridge.h"
#include "sass_types/factory.h"
#include "sass_types/value.h"

#include <stdio.h>

Sass_Value* CustomFunctionBridge::post_process_return_value(v8::Local<v8::Value> val) const {
  SassTypes::Value *v_;
  if (val->IsNull()) {
    return sass_make_null();
  } else if ((v_ = SassTypes::Factory::unwrap(val))) {
    return v_->get_sass_value();
  } else {
    return sass_make_error("A SassValue object was expected.");
  }
}

std::vector<v8::Local<v8::Value>> CustomFunctionBridge::pre_process_args(std::vector<void*> in) const {
  std::vector<v8::Local<v8::Value>> argv = std::vector<v8::Local<v8::Value>>();

  for (void* value : in) {
    Sass_Value *vptr = static_cast<Sass_Value*>(value);
    v8::Local<v8::Value> item;

    if (sass_value_is_null(vptr)) {
        item = Nan::Null();
    } else {
        item = SassTypes::Factory::create(vptr)->get_js_object();
    }
    argv.push_back(item);
  }

  return argv;
}
