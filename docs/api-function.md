Sass functions are used to define new custom functions callable by Sass code. They are also used to overload debug or error statements. You can also define a fallback function, which is called for every unknown function found in the Sass code. Functions get passed zero or more `Sass_Values` (a `Sass_List` value) and they must also return a `Sass_Value`. Return a `Sass_Error` if you want to signal an error.

## Special signatures

- `*` - Fallback implementation
- `@warn` - Overload warn statements
- `@error` - Overload error statements
- `@debug` - Overload debug statements

Note: The fallback implementation will be given the name of the called function as the first argument, before all the original function arguments. These features are pretty new and should be considered experimental.

### Basic Usage

```C
#include "sass/functions.h"
```

## Sass Function API

```C
// Forward declaration
struct Sass_C_Function_Descriptor;

// Typedef defining null terminated list of custom callbacks
typedef struct Sass_C_Function_Descriptor* (*Sass_C_Function_List);
typedef struct Sass_C_Function_Descriptor (*Sass_C_Function_Callback);
// Typedef defining custom function prototype and its return value type
typedef union Sass_Value*(*Sass_C_Function) (const union Sass_Value*, void* cookie);

// Creators for sass function list and function descriptors
Sass_C_Function_List sass_make_function_list (size_t length);
Sass_C_Function_Callback sass_make_function (const char* signature, Sass_C_Function fn, void* cookie);

// Setters and getters for callbacks on function lists
Sass_C_Function_Callback sass_function_get_list_entry(Sass_C_Function_List list, size_t pos);
void sass_function_set_list_entry(Sass_C_Function_List list, size_t pos, Sass_C_Function_Callback cb);

// Getters for custom function descriptors
const char* sass_function_get_signature (Sass_C_Function_Callback fn);
Sass_C_Function sass_function_get_function (Sass_C_Function_Callback fn);
void* sass_function_get_cookie (Sass_C_Function_Callback fn);
```

### More links

- [Sass Function Example](api-function-example.md)
- [Sass Function Internal](api-function-internal.md)

