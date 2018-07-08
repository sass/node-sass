#ifndef COMMON_H
#define COMMON_H

#define NAPI_DISABLE_CPP_EXCEPTIONS 1

#include <cassert>

#define CHECK_NAPI_RESULT(condition) \
  do { napi_status status = (condition); assert(status == napi_ok || status == napi_pending_exception); } while(0)

#endif // COMMON_H
