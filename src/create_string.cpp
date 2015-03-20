#include "create_string.h"

char* CreateString(Local<Value> value) {
  if (value->IsNull() || !value->IsString()) {
    return 0;
  }

  String::Utf8Value string(value);
  char *str = (char *)malloc(string.length() + 1);
  strcpy(str, *string);
  return str;
}
