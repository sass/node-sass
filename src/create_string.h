#ifndef CREATE_STRING_H
#define CREATE_STRING_H

#define NAPI_DISABLE_CPP_EXCEPTIONS 1

#include <napi.h>

char* create_string(napi_env e, napi_value v);

#endif
