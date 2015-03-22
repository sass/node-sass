#include "null.h"

using namespace v8;

namespace SassTypes
{
  Null::Null(Sass_Value* v) : CoreValue(v) {}

  Sass_Value* Null::construct(const std::vector<Local<v8::Value>> raw_val) {
    return sass_make_null();
  }

  void Null::initPrototype(Handle<ObjectTemplate>) {}
}
