#include "value.h"

using namespace v8;

namespace SassTypes
{
  Value::Value(Sass_Value* v) {
    this->value = sass_clone_value(v);
  }

  Value::~Value() {
    sass_delete_value(this->value);
  }
}
