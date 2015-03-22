#ifndef SASS_TYPES_STRING_H
#define SASS_TYPES_STRING_H

#include <nan.h>
#include <sass_values.h>
#include "../create_string.h"
#include "core_value.h"

namespace SassTypes
{
  using namespace v8;

  class String : public CoreValue<String> {
    public:
      String(Sass_Value*);
      static char const* get_constructor_name() { return "SassString"; }
      static Sass_Value* construct(const std::vector<Local<v8::Value>>);

      static void initPrototype(Handle<ObjectTemplate>);

      static NAN_METHOD(GetValue);
      static NAN_METHOD(SetValue);
  };
}

#endif
