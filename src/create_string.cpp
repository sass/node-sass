#include <stdlib.h>
#include <string.h>
#include "create_string.h"

// node-sass only builds with MSVC 2013 which doesn't appear to have char16_t defined
#define char16_t wchar_t

#include <node_jsvmapi.h>

#define CHECK_NAPI_RESULT(condition) do { if((condition) != napi_ok) { return nullptr; } } while(0)

char* create_string(napi_env e, napi_value v) {
  napi_valuetype t;  
  CHECK_NAPI_RESULT(napi_get_type_of_value(e, v, &t));

  if (t != napi_string) {
      return nullptr;
  }

  int len;
  CHECK_NAPI_RESULT(napi_get_value_string_utf8_length(e, v, &len));

  char* str = (char *)malloc(len + 1);
  int written;
  CHECK_NAPI_RESULT(napi_get_value_string_utf8(e, v, str, len + 1, &written));

  if (len + 1 != written) {
    free(str);
    return nullptr;
  }

  return str;
}
