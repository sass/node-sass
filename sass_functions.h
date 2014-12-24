#ifndef SASS_C_FUNCTIONS
#define SASS_C_FUNCTIONS

#include <stddef.h>
#include <stdbool.h>
#include "sass.h"

#ifdef __cplusplus
extern "C" {
#endif


// Forward declaration
struct Sass_Import;

// Forward declaration
struct Sass_C_Import_Descriptor;

// Typedef defining the custom importer callback
typedef struct Sass_C_Import_Descriptor (*Sass_C_Import_Callback);
// Typedef defining the importer c function prototype
typedef struct Sass_Import** (*Sass_C_Import_Fn) (const char* url, const char* prev, void* cookie);

// Creators for custom importer callback (with some additional pointer)
// The pointer is mostly used to store the callback into the actual binding
ADDAPI Sass_C_Import_Callback ADDCALL sass_make_importer (Sass_C_Import_Fn, void* cookie);

// Getters for import function descriptors
ADDAPI Sass_C_Import_Fn ADDCALL sass_import_get_function (Sass_C_Import_Callback fn);
ADDAPI void* ADDCALL sass_import_get_cookie (Sass_C_Import_Callback fn);

// Deallocator for associated memory
ADDAPI void ADDCALL sass_delete_importer (Sass_C_Import_Callback fn);

// Creator for sass custom importer return argument list
ADDAPI struct Sass_Import** ADDCALL sass_make_import_list (size_t length);
// Creator for a single import entry returned by the custom importer inside the list
ADDAPI struct Sass_Import* ADDCALL sass_make_import_entry (const char* path, char* source, char* srcmap);
ADDAPI struct Sass_Import* ADDCALL sass_make_import (const char* path, const char* base, char* source, char* srcmap);

// Setters to insert an entry into the import list (you may also use [] access directly)
// Since we are dealing with pointers they should have a guaranteed and fixed size
ADDAPI void ADDCALL sass_import_set_list_entry (struct Sass_Import** list, size_t idx, struct Sass_Import* entry);
ADDAPI struct Sass_Import* ADDCALL sass_import_get_list_entry (struct Sass_Import** list, size_t idx);

// Getters for import entry
ADDAPI const char* ADDCALL sass_import_get_path (struct Sass_Import*);
ADDAPI const char* ADDCALL sass_import_get_base (struct Sass_Import*);
ADDAPI const char* ADDCALL sass_import_get_source (struct Sass_Import*);
ADDAPI const char* ADDCALL sass_import_get_srcmap (struct Sass_Import*);
// Explicit functions to take ownership of these items
// The property on our struct will be reset to NULL
ADDAPI char* ADDCALL sass_import_take_source (struct Sass_Import*);
ADDAPI char* ADDCALL sass_import_take_srcmap (struct Sass_Import*);

// Deallocator for associated memory (incl. entries)
ADDAPI void ADDCALL sass_delete_import_list (struct Sass_Import**);
// Just in case we have some stray import structs
ADDAPI void ADDCALL sass_delete_import (struct Sass_Import*);


// Forward declaration
struct Sass_C_Function_Descriptor;

// Typedef defining null terminated list of custom callbacks
typedef struct Sass_C_Function_Descriptor* (*Sass_C_Function_List);
typedef struct Sass_C_Function_Descriptor (*Sass_C_Function_Callback);
// Typedef defining custom function prototype and its return value type
typedef union Sass_Value*(*Sass_C_Function) (const union Sass_Value*, void* cookie);


// Creators for sass function list and function descriptors
ADDAPI Sass_C_Function_List ADDCALL sass_make_function_list (size_t length);
ADDAPI Sass_C_Function_Callback ADDCALL sass_make_function (const char* signature, Sass_C_Function fn, void* cookie);

// Setters and getters for callbacks on function lists
ADDAPI Sass_C_Function_Callback ADDCALL sass_function_get_list_entry(Sass_C_Function_List list, size_t pos);
ADDAPI void ADDCALL sass_function_set_list_entry(Sass_C_Function_List list, size_t pos, Sass_C_Function_Callback cb);

// Getters for custom function descriptors
ADDAPI const char* ADDCALL sass_function_get_signature (Sass_C_Function_Callback fn);
ADDAPI Sass_C_Function ADDCALL sass_function_get_function (Sass_C_Function_Callback fn);
ADDAPI void* ADDCALL sass_function_get_cookie (Sass_C_Function_Callback fn);


#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif
