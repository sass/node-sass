#ifndef SASS_TYPES_BOOLEAN_H
#define SASS_TYPES_BOOLEAN_H

#include <nan.h>
#include <sass_values.h>
#include "core_value.h"

namespace SassTypes
{
  using namespace v8;

  class Boolean : public CoreValue<Boolean> {
    public:
      Boolean(Sass_Value*);
      static char const* get_constructor_name() { return "SassBoolean"; }
      static Sass_Value* construct(const std::vector<Local<v8::Value>>);

      static void initPrototype(v8::Handle<v8::ObjectTemplate>);

      static NAN_METHOD(GetValue);
      static NAN_METHOD(SetValue);
  };
}

#endif
