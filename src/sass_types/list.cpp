#include <nan.h>
#include "list.h"

namespace SassTypes
{
  List::List(Sass_Value* v) : SassValueWrapper(v) {}

  Sass_Value* List::construct(const std::vector<v8::Local<v8::Value>> raw_val) {
    size_t length = 0;
    bool comma = true;

    if (raw_val.size() >= 1) {
      if (!raw_val[0]->IsNumber()) {
        throw std::invalid_argument("First argument should be an integer.");
      }

      length = raw_val[0]->ToInt32()->Value();

      if (raw_val.size() >= 2) {
        if (!raw_val[1]->IsBoolean()) {
          throw std::invalid_argument("Second argument should be a boolean.");
        }

        comma = raw_val[1]->ToBoolean()->Value();
      }
    }

    return sass_make_list(length, comma ? SASS_COMMA : SASS_SPACE);
  }

  void List::initPrototype(v8::Local<v8::FunctionTemplate> proto) {
    Nan::SetPrototypeMethod(proto, "getLength", GetLength);
    Nan::SetPrototypeMethod(proto, "getSeparator", GetSeparator);
    Nan::SetPrototypeMethod(proto, "setSeparator", SetSeparator);
    Nan::SetPrototypeMethod(proto, "getValue", GetValue);
    Nan::SetPrototypeMethod(proto, "setValue", SetValue);
  }

  NAN_METHOD(List::GetValue) {

    if (info.Length() != 1) {
      return Nan::ThrowError(Nan::New("Expected just one argument").ToLocalChecked());
    }

    if (!info[0]->IsNumber()) {
      return Nan::ThrowError(Nan::New("Supplied index should be an integer").ToLocalChecked());
    }

    Sass_Value* list = unwrap(info.This())->value;
    size_t index = info[0]->ToInt32()->Value();


    if (index >= sass_list_get_length(list)) {
      return Nan::ThrowError(Nan::New("Out of bound index").ToLocalChecked());
    }

    info.GetReturnValue().Set(Factory::create(sass_list_get_value(list, info[0]->ToInt32()->Value()))->get_js_object());
  }

  NAN_METHOD(List::SetValue) {
    if (info.Length() != 2) {
      return Nan::ThrowError(Nan::New("Expected two arguments").ToLocalChecked());
    }

    if (!info[0]->IsNumber()) {
      return Nan::ThrowError(Nan::New("Supplied index should be an integer").ToLocalChecked());
    }

    if (!info[1]->IsObject()) {
      return Nan::ThrowError(Nan::New("Supplied value should be a SassValue object").ToLocalChecked());
    }

    Value* sass_value = Factory::unwrap(info[1]);
    sass_list_set_value(unwrap(info.This())->value, info[0]->ToInt32()->Value(), sass_value->get_sass_value());
  }

  NAN_METHOD(List::GetSeparator) {
    info.GetReturnValue().Set(sass_list_get_separator(unwrap(info.This())->value) == SASS_COMMA);
  }

  NAN_METHOD(List::SetSeparator) {
    if (info.Length() != 1) {
      return Nan::ThrowError(Nan::New("Expected just one argument").ToLocalChecked());
    }

    if (!info[0]->IsBoolean()) {
      return Nan::ThrowError(Nan::New("Supplied value should be a boolean").ToLocalChecked());
    }

    sass_list_set_separator(unwrap(info.This())->value, info[0]->ToBoolean()->Value() ? SASS_COMMA : SASS_SPACE);
  }

  NAN_METHOD(List::GetLength) {
    info.GetReturnValue().Set(Nan::New<v8::Number>(sass_list_get_length(unwrap(info.This())->value)));
  }
}
