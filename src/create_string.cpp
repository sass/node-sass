#include <stdlib.h>
#include <string.h>
#include "create_string.h"
#include <napi.h>

#define CHECK_NAPI_RESULT_RETURN_NULL(condition) do { if((condition) != napi_ok) { return nullptr; } } while(0)

char* create_string(napi_env e, napi_value v) {
  napi_valuetype t;
  CHECK_NAPI_RESULT_RETURN_NULL(napi_typeof(e, v, &t));

  if (t != napi_string) {
    return nullptr;
  }

  size_t len;
  CHECK_NAPI_RESULT_RETURN_NULL(napi_get_value_string_utf8(e, v, NULL, 0, &len));

  char* str = (char *)malloc(len + 1);
  size_t written;
  CHECK_NAPI_RESULT_RETURN_NULL(napi_get_value_string_utf8(e, v, str, len + 1, &written));

  if (len != written) {
    free(str);
    return nullptr;
  }

  return str;
}
