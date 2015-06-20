#include <nan.h>
#include "string.h"
#include "../create_string.h"

using namespace v8;

namespace SassTypes
{
  String::String(Sass_Value* v) : SassValueWrapper(v) {}

  Sass_Value* String::construct(const std::vector<Local<v8::Value>> raw_val) {
    char const* value = "";

    if (raw_val.size() >= 1) {
      if (!raw_val[0]->IsString()) {
        throw std::invalid_argument("Argument should be a string.");
      }

      value = create_string(raw_val[0]);
    }

    return sass_make_string(value);
  }

  void String::initPrototype(Handle<ObjectTemplate> proto) {
    proto->Set(NanNew("getValue"), NanNew<FunctionTemplate>(GetValue)->GetFunction());
    proto->Set(NanNew("setValue"), NanNew<FunctionTemplate>(SetValue)->GetFunction());
    proto->Set(NanNew("isQuoted"), NanNew<FunctionTemplate>(IsQuoted)->GetFunction());
    proto->Set(NanNew("setQuoted"), NanNew<FunctionTemplate>(SetQuoted)->GetFunction());
  }

  NAN_METHOD(String::GetValue) {
    NanScope();
    NanReturnValue(NanNew(sass_string_get_value(unwrap(args.This())->value)));
  }

  NAN_METHOD(String::SetValue) {
    if (args.Length() != 1) {
      return NanThrowError(NanNew("Expected just one argument"));
    }

    if (!args[0]->IsString()) {
      return NanThrowError(NanNew("Supplied value should be a string"));
    }

    sass_string_set_value(unwrap(args.This())->value, create_string(args[0]));
    NanReturnUndefined();
  }

  NAN_METHOD(String::IsQuoted) {
    NanScope();
    NanReturnValue(NanNew(sass_string_is_quoted(unwrap(args.This())->value)));
  }

  NAN_METHOD(String::SetQuoted) {
    if (args.Length() != 1) {
      return NanThrowError(NanNew("Expected just one argument"));
    }

    if (!args[0]->IsBoolean()) {
      return NanThrowError(NanNew("Supplied value should be a boolean"));
    }

    sass_string_set_quoted(unwrap(args.This())->value, args[0]->ToBoolean()->Value());
    NanReturnUndefined();
  }
}
