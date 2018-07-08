#ifndef SASS_TYPES_FACTORY_H
#define SASS_TYPES_FACTORY_H

#include <sass/values.h>
#include "value.h"
#include <napi.h>

namespace SassTypes
{
  // This is the guru that knows everything about instantiating the right subclass of SassTypes::Value
  // to wrap a given Sass_Value object.
  class Factory {
    public:
      static void initExports(napi_env, napi_value);
      static Value* create(napi_env, Sass_Value*);
      static Value* unwrap(napi_env, napi_value);
  };
}

#endif
