#ifndef SASS_C_CONTEXT
#define SASS_C_CONTEXT

#include <stddef.h>
#include <stdbool.h>
#include "sass.h"

#ifdef __cplusplus
extern "C" {
#endif


// Forward declaration
struct Sass_Compiler;

// Forward declaration
struct Sass_Options;
struct Sass_Context; // : Sass_Options
struct Sass_File_Context; // : Sass_Context
struct Sass_Data_Context; // : Sass_Context

// Create and initialize an option struct
struct Sass_Options* sass_make_options (void);
// Create and initialize a specific context
struct Sass_File_Context* sass_make_file_context (const char* input_path);
struct Sass_Data_Context* sass_make_data_context (char* source_string);

// Call the compilation step for the specific context
int sass_compile_file_context (struct Sass_File_Context* ctx);
int sass_compile_data_context (struct Sass_Data_Context* ctx);

// Create a sass compiler instance for more control
struct Sass_Compiler* sass_make_file_compiler (struct Sass_File_Context* file_ctx);
struct Sass_Compiler* sass_make_data_compiler (struct Sass_Data_Context* data_ctx);

// Execute the different compilation steps individually
// Usefull if you only want to query the included files
int sass_compiler_parse(struct Sass_Compiler* compiler);
int sass_compiler_execute(struct Sass_Compiler* compiler);

// Release all memory allocated with the compiler
// This does _not_ include any contexts or options
void sass_delete_compiler(struct Sass_Compiler* compiler);

// Release all memory allocated and also ourself
void sass_delete_file_context (struct Sass_File_Context* ctx);
void sass_delete_data_context (struct Sass_Data_Context* ctx);

// Getters for context from specific implementation
struct Sass_Context* sass_file_context_get_context (struct Sass_File_Context* file_ctx);
struct Sass_Context* sass_data_context_get_context (struct Sass_Data_Context* data_ctx);

// Getters for context options from Sass_Context
struct Sass_Options* sass_context_get_options (struct Sass_Context* ctx);
struct Sass_Options* sass_file_context_get_options (struct Sass_File_Context* file_ctx);
struct Sass_Options* sass_data_context_get_options (struct Sass_Data_Context* data_ctx);
void sass_file_context_set_options (struct Sass_File_Context* file_ctx, struct Sass_Options* opt);
void sass_data_context_set_options (struct Sass_Data_Context* data_ctx, struct Sass_Options* opt);


// Getters for options
int sass_option_get_precision (struct Sass_Options* options);
enum Sass_Output_Style sass_option_get_output_style (struct Sass_Options* options);
bool sass_option_get_source_comments (struct Sass_Options* options);
bool sass_option_get_source_map_embed (struct Sass_Options* options);
bool sass_option_get_source_map_contents (struct Sass_Options* options);
bool sass_option_get_omit_source_map_url (struct Sass_Options* options);
bool sass_option_get_is_indented_syntax_src (struct Sass_Options* options);
const char* sass_option_get_input_path (struct Sass_Options* options);
const char* sass_option_get_output_path (struct Sass_Options* options);
const char* sass_option_get_image_path (struct Sass_Options* options);
const char* sass_option_get_include_path (struct Sass_Options* options);
const char* sass_option_get_source_map_file (struct Sass_Options* options);
Sass_C_Function_List sass_option_get_c_functions (struct Sass_Options* options);
Sass_C_Import_Callback sass_option_get_importer (struct Sass_Options* options);

// Setters for options
void sass_option_set_precision (struct Sass_Options* options, int precision);
void sass_option_set_output_style (struct Sass_Options* options, enum Sass_Output_Style output_style);
void sass_option_set_source_comments (struct Sass_Options* options, bool source_comments);
void sass_option_set_source_map_embed (struct Sass_Options* options, bool source_map_embed);
void sass_option_set_source_map_contents (struct Sass_Options* options, bool source_map_contents);
void sass_option_set_omit_source_map_url (struct Sass_Options* options, bool omit_source_map_url);
void sass_option_set_is_indented_syntax_src (struct Sass_Options* options, bool is_indented_syntax_src);
void sass_option_set_input_path (struct Sass_Options* options, const char* input_path);
void sass_option_set_output_path (struct Sass_Options* options, const char* output_path);
void sass_option_set_image_path (struct Sass_Options* options, const char* image_path);
void sass_option_set_include_path (struct Sass_Options* options, const char* include_path);
void sass_option_set_source_map_file (struct Sass_Options* options, const char* source_map_file);
void sass_option_set_c_functions (struct Sass_Options* options, Sass_C_Function_List c_functions);
void sass_option_set_importer (struct Sass_Options* options, Sass_C_Import_Callback importer);


// Getter for context
const char* sass_context_get_output_string (struct Sass_Context* ctx);
int sass_context_get_error_status (struct Sass_Context* ctx);
const char* sass_context_get_error_json (struct Sass_Context* ctx);
const char* sass_context_get_error_message (struct Sass_Context* ctx);
const char* sass_context_get_source_map_string (struct Sass_Context* ctx);
char** sass_context_get_included_files (struct Sass_Context* ctx);


// Setters for specific data context option
// const char* sass_data_context_get_source_string (struct Sass_Data_Context* ctx);
void sass_data_context_set_source_string (struct Sass_Data_Context* ctx, char* source_string);

// Push function for include paths (no manipulation support for now)
void sass_option_push_include_path (struct Sass_Options* options, const char* path);


#ifdef __cplusplus
}
#endif

#endif