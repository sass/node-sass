#ifndef SASS_C_FUNCTIONS
#define SASS_C_FUNCTIONS

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// Forward declaration
struct Sass_C_Import_Descriptor;

// Typedef defining the custom importer callback
typedef struct Sass_C_Import_Descriptor (*Sass_C_Import_Callback);
// Typedef defining the importer c function prototype
typedef struct Sass_Import** (*Sass_C_Import_Fn) (const char* url, void* cookie);

// Creators for custom importer callback (with some additional pointer)
// The pointer is mostly used to store the callback into the actual binding
Sass_C_Import_Callback sass_make_importer (Sass_C_Import_Fn, void* cookie);

// Getters for import function descriptors
Sass_C_Import_Fn sass_import_get_function (Sass_C_Import_Callback fn);
void* sass_import_get_cookie (Sass_C_Import_Callback fn);


// Creator for sass custom importer return argument list
struct Sass_Import** sass_make_import_list (size_t length);
// Creator for a single import entry returned by the custom importer inside the list
struct Sass_Import* sass_make_import_entry (const char* path, char* source, const char* srcmap);

// Setters to insert an entry into the import list (you may also use [] access directly)
// Since we are dealing with pointers they should have a guaranteed and fixed size
void sass_import_set_list_entry (struct Sass_Import** list, size_t idx, struct Sass_Import* entry);
struct Sass_Import* sass_import_get_list_entry (struct Sass_Import** list, size_t idx);

// Getters for import entry
const char*sass_import_get_path (struct Sass_Import*);
const char*sass_import_get_source (struct Sass_Import*);
const char*sass_import_get_srcmap (struct Sass_Import*);

// Deallocator for associated memory (incl. entries)
void sass_delete_import_list (struct Sass_Import**);


// Forward declaration
struct Sass_C_Function_Descriptor;

// Typedef defining null terminated list of custom callbacks
typedef struct Sass_C_Function_Descriptor* (*Sass_C_Function_List);
typedef struct Sass_C_Function_Descriptor (*Sass_C_Function_Callback);
// Typedef defining custom function prototype and its return value type
typedef union Sass_Value*(*Sass_C_Function) (union Sass_Value*, void *cookie);


// Creators for sass function list and function descriptors
Sass_C_Function_List sass_make_function_list (size_t length);
Sass_C_Function_Callback sass_make_function (const char* signature, Sass_C_Function fn, void* cookie);

// Getters for custom function descriptors
const char* sass_function_get_signature (Sass_C_Function_Callback fn);
Sass_C_Function sass_function_get_function (Sass_C_Function_Callback fn);
void* sass_function_get_cookie (Sass_C_Function_Callback fn);


#ifdef __cplusplus
}
#endif

#endif
