#ifndef SASS_TYPES_LIST_H
#define SASS_TYPES_LIST_H

#include <nan.h>
#include <sass_values.h>
#include "core_value.h"

namespace SassTypes
{
  using namespace v8;

  class List : public CoreValue<List> {
    public:
      List(Sass_Value*);
      static char const* get_constructor_name() { return "SassList"; }
      static Sass_Value* construct(const std::vector<Local<v8::Value>>);

      static void initPrototype(Handle<ObjectTemplate>);

      static NAN_METHOD(GetValue);
      static NAN_METHOD(SetValue);
      static NAN_METHOD(GetSeparator);
      static NAN_METHOD(SetSeparator);
      static NAN_METHOD(GetLength);
  };
}

#endif
