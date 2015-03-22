#ifndef SASS_TYPES_ERROR_H
#define SASS_TYPES_ERROR_H

#include <sass_values.h>
#include "../create_string.h"
#include "core_value.h"

namespace SassTypes
{
  using namespace v8;

  class Error : public CoreValue<Error> {
    public:
      Error(Sass_Value*);
      static char const* get_constructor_name() { return "SassError"; }
      static Sass_Value* construct(const std::vector<Local<v8::Value>>);

      static void initPrototype(Handle<ObjectTemplate>);
  };
}

#endif
