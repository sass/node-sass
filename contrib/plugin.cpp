#include <cstring>
#include <iostream>
#include <stdint.h>
#include "sass_values.h"

// gcc: g++ -shared plugin.cpp -o plugin.so -fPIC -Llib -lsass
// mingw: g++ -shared plugin.cpp -o plugin.dll -Llib -lsass

union Sass_Value* call_fn_foo(const union Sass_Value* s_args, void* cookie)
{
  // we actually abuse the void* to store an "int"
  return sass_make_number((intptr_t)cookie, "px");
}

extern "C" const char* ADDCALL libsass_get_version() {
  return libsass_version();
}

extern "C" Sass_C_Function_List ADDCALL libsass_load_functions()
{
  // allocate a custom function caller
  Sass_C_Function_Call fn_foo =
    sass_make_function("foo()", call_fn_foo, (void*)42);
  // create list of all custom functions
  Sass_C_Function_List fn_list = sass_make_function_list(1);
  // put the only function in this plugin to the list
  sass_function_set_list_entry(fn_list, 0, fn_foo);
  // return the list
  return fn_list;
}
