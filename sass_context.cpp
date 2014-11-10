#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "json.hpp"
#include "context.hpp"
#include "sass_values.h"
#include "sass_context.h"

extern "C" {
  using namespace std;

  // Input behaviours
  enum Sass_Input_Style {
    SASS_CONTEXT_NULL,
    SASS_CONTEXT_FILE,
    SASS_CONTEXT_DATA,
    SASS_CONTEXT_FOLDER
  };

  // simple linked list
  struct string_list {
    string_list* next;
    const char*  string;
  };

  // sass config options structure
  struct Sass_Options {

    // Precision for fractional numbers
    int precision;

    // Output style for the generated css code
    // A value from above SASS_STYLE_* constants
    enum Sass_Output_Style output_style;

    // Emit comments in the generated CSS indicating
    // the corresponding source line.
    bool source_comments;

    // embed sourceMappingUrl as data uri
    bool source_map_embed;

    // embed include contents in maps
    bool source_map_contents;

    // Disable sourceMappingUrl in css output
    bool omit_source_map_url;

    // Treat source_string as sass (as opposed to scss)
    bool is_indented_syntax_src;

    // The input path is used for source map
    // generation. It can be used to define
    // something with string compilation or to
    // overload the input file path. It is
    // set to "stdin" for data contexts and
    // to the input file on file contexts.
    const char* input_path;

    // The output path is used for source map
    // generation. Libsass will not write to
    // this file, it is just used to create
    // information in source-maps etc.
    const char* output_path;

    // For the image-url Sass function
    const char* image_path;

    // Colon-separated list of paths
    // Semicolon-separated on Windows
    // Maybe use array interface instead?
    const char* include_path;

    // Include path (linked string list)
    struct string_list* include_paths;

    // Path to source map file
    // Enables source map generation
    // Used to create sourceMappingUrl
    const char* source_map_file;

    // Custom functions that can be called from sccs code
    Sass_C_Function_List c_functions;

    // Callback to overload imports
    Sass_C_Import_Callback importer;

  };

  // base for all contexts
  struct Sass_Context : Sass_Options
  {

    // store context type info
    enum Sass_Input_Style type;

    // generated output data
    char* output_string;

    // generated source map json
    char* source_map_string;

    // error status
    int error_status;
    char* error_json;
    char* error_message;

    // report imported files
    char** included_files;

  };

  // struct for file compilation
  struct Sass_File_Context : Sass_Context {

    // no additional fields required
    // input_path is already on options

  };

  // struct for data compilation
  struct Sass_Data_Context : Sass_Context {

    // provided source string
    char* source_string;

  };


  #define IMPLEMENT_SASS_OPTION_GETTER(type, option) \
    type sass_option_get_##option (struct Sass_Options* options) { return options->option; }
  #define IMPLEMENT_SASS_OPTION_SETTER(type, option) \
    void sass_option_set_##option (struct Sass_Options* options, type option) { options->option = option; }
  #define IMPLEMENT_SASS_OPTION_ACCESSOR(type, option) \
    IMPLEMENT_SASS_OPTION_GETTER(type, option) \
    IMPLEMENT_SASS_OPTION_SETTER(type, option)

  #define IMPLEMENT_SASS_CONTEXT_GETTER(type, option) \
    type sass_context_get_##option (struct Sass_Context* ctx) { return ctx->option; }
  #define IMPLEMENT_SASS_FILE_CONTEXT_SETTER(type, option) \
    void sass_file_context_set_##option (struct Sass_File_Context* ctx, type option) { ctx->option = option; }
  #define IMPLEMENT_SASS_DATA_CONTEXT_SETTER(type, option) \
    void sass_data_context_set_##option (struct Sass_Data_Context* ctx, type option) { ctx->option = option; }

  // helper for safe access to c_ctx
  static const char* safe_str (const char* str) {
    return str == NULL ? "" : str;
  }

  static void copy_strings(const std::vector<std::string>& strings, char*** array, int skip = 0) {
    int num = static_cast<int>(strings.size());
    char** arr = (char**) malloc(sizeof(char*)* num + 1);

    for(int i = skip; i < num; i++) {
      arr[i-skip] = (char*) malloc(sizeof(char) * strings[i].size() + 1);
      std::copy(strings[i].begin(), strings[i].end(), arr[i-skip]);
      arr[i-skip][strings[i].size()] = '\0';
    }

    arr[num-skip] = 0;
    *array = arr;
  }

  static void free_string_array(char ** arr) {
    if(!arr)
        return;

    char **it = arr;
    while (it && (*it)) {
      free(*it);
      ++it;
    }

    free(arr);
  }

  // generic compilation function (not exported, use file/data compile instead)
  static int sass_compile_context (Sass_Context* c_ctx, Sass::Context::Data cpp_opt)
  {

    using namespace Sass;
    try {

      // get input/output path from options
      string input_path = safe_str(c_ctx->input_path);
      string output_path = safe_str(c_ctx->output_path);
      // maybe we can extract an output path from input path
      if (output_path == "" && input_path != "") {
        int lastindex = static_cast<int>(input_path.find_last_of("."));
        output_path = (lastindex > -1 ? input_path.substr(0, lastindex) : input_path) + ".css";
      }

      // convert include path linked list to static array
      struct string_list* cur = c_ctx->include_paths;
      // very poor loop to get the length of the linked list
      size_t length = 0; while (cur) { length ++; cur = cur->next; }
      // create char* array to hold all paths plus null terminator
      const char** include_paths = (const char**) calloc(length + 1, sizeof(char*));
      // reset iterator
      cur = c_ctx->include_paths;
      // copy over the paths
      for (size_t i = 0; cur; i++) {
        include_paths[i] = cur->string;
        cur = cur->next;
      }

      // transfer the options to c++
      cpp_opt.output_path(output_path)
             .output_style((Output_Style) c_ctx->output_style)
             .is_indented_syntax_src(c_ctx->is_indented_syntax_src)
             .source_comments(c_ctx->source_comments)
             .source_map_file(safe_str(c_ctx->source_map_file))
             .source_map_embed(c_ctx->source_map_embed)
             .source_map_contents(c_ctx->source_map_contents)
             .omit_source_map_url(c_ctx->omit_source_map_url)
             .image_path(safe_str(c_ctx->image_path))
             .include_paths_c_str(c_ctx->include_path)
             .importer(c_ctx->importer)
             .include_paths_array(include_paths)
             .include_paths(vector<string>())
             .precision(c_ctx->precision ? c_ctx->precision : 5);

      // create new c++ Context
      Context cpp_ctx(cpp_opt);
      // free intermediate data
      free(include_paths);

      // register our custom functions
      if (c_ctx->c_functions) {
        Sass_C_Function_List this_func_data = c_ctx->c_functions;
        while ((this_func_data) && (*this_func_data)) {
          cpp_ctx.c_functions.push_back((*this_func_data));
          ++this_func_data;
        }
      }

      // reset error status
      c_ctx->error_json = 0;
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;

      // dispatch to the correct render function
      if (c_ctx->type == SASS_CONTEXT_FILE) {
        c_ctx->output_string = cpp_ctx.compile_file();
      } else if (c_ctx->type == SASS_CONTEXT_DATA) {
        if (input_path == "") { c_ctx->output_string = cpp_ctx.compile_string(); }
        else { c_ctx->output_string = cpp_ctx.compile_string(input_path); }
      }

      // generate source map json and store on context
      c_ctx->source_map_string = cpp_ctx.generate_source_map();
      // copy the included files on to the context (dont forget to free)
      copy_strings(cpp_ctx.get_included_files(1), &c_ctx->included_files, 1);

    }
    catch (Error& e) {
      stringstream msg_stream;
      JsonNode* json_err = json_mkobject();
      json_append_member(json_err, "status", json_mknumber(1));
      json_append_member(json_err, "path", json_mkstring(e.path.c_str()));
      json_append_member(json_err, "line", json_mknumber(e.position.line));
      json_append_member(json_err, "column", json_mknumber(e.position.column));
      json_append_member(json_err, "message", json_mkstring(e.message.c_str()));
      msg_stream << e.path << ":" << e.position.line << ": " << e.message << endl;
      c_ctx->error_json = json_stringify(json_err, "\t");;
      c_ctx->error_message = strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch(bad_alloc& ba) {
      stringstream msg_stream;
      JsonNode* json_err = json_mkobject();
      msg_stream << "Unable to allocate memory: " << ba.what() << endl;
      json_append_member(json_err, "status", json_mknumber(2));
      json_append_member(json_err, "message", json_mkstring(ba.what()));
      c_ctx->error_json = json_stringify(json_err, "\t");;
      c_ctx->error_message = strdup(msg_stream.str().c_str());
      c_ctx->error_status = 2;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (std::exception& e) {
      stringstream msg_stream;
      JsonNode* json_err = json_mkobject();
      msg_stream << "Error: " << e.what() << endl;
      json_append_member(json_err, "status", json_mknumber(3));
      json_append_member(json_err, "message", json_mkstring(e.what()));
      c_ctx->error_json = json_stringify(json_err, "\t");;
      c_ctx->error_message = strdup(msg_stream.str().c_str());
      c_ctx->error_status = 3;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (string& e) {
      stringstream msg_stream;
      JsonNode* json_err = json_mkobject();
      msg_stream << "Error: " << e << endl;
      json_append_member(json_err, "status", json_mknumber(4));
      json_append_member(json_err, "message", json_mkstring(e.c_str()));
      c_ctx->error_json = json_stringify(json_err, "\t");;
      c_ctx->error_message = strdup(msg_stream.str().c_str());
      c_ctx->error_status = 4;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (...) {
      stringstream msg_stream;
      JsonNode* json_err = json_mkobject();
      msg_stream << "Unknown error occurred" << endl;
      json_append_member(json_err, "status", json_mknumber(5));
      json_append_member(json_err, "message", json_mkstring("unknown"));
      c_ctx->error_json = json_stringify(json_err, "\t");;
      c_ctx->error_message = strdup(msg_stream.str().c_str());
      c_ctx->error_status = 5;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    return c_ctx->error_status;
  }

  Sass_Options* sass_make_options (void)
  {
    return (struct Sass_Options*) calloc(1, sizeof(struct Sass_Options));
  }

  Sass_File_Context* sass_make_file_context(const char* input_path)
  {
    struct Sass_File_Context* ctx = (struct Sass_File_Context*) calloc(1, sizeof(struct Sass_File_Context));
    ctx->type = SASS_CONTEXT_FILE;
    ctx->input_path = input_path;
    return ctx;
  }

  Sass_Data_Context* sass_make_data_context(char* source_string)
  {
    struct Sass_Data_Context* ctx = (struct Sass_Data_Context*) calloc(1, sizeof(struct Sass_Data_Context));
    ctx->type = SASS_CONTEXT_DATA;
    ctx->source_string = source_string;
    return ctx;
  }

  int sass_compile_data_context(Sass_Data_Context* data_ctx)
  {
    using namespace Sass;
    Sass_Context* c_ctx = data_ctx;
    if (!c_ctx->input_path) {
      // use a default value which
      // will be seen relative to cwd
      c_ctx->input_path = "stdin";
    }
    Context::Data cpp_opt = Context::Data();
    cpp_opt.source_c_str(data_ctx->source_string);
    return sass_compile_context(c_ctx, cpp_opt);
  }

  int sass_compile_file_context(Sass_File_Context* file_ctx)
  {
    using namespace Sass;
    Sass_Context* c_ctx = file_ctx;
    Context::Data cpp_opt = Context::Data();
    cpp_opt.entry_point(file_ctx->input_path);
    return sass_compile_context(c_ctx, cpp_opt);
  }

  // helper function, not exported, only accessible locally
  static void sass_clear_options (struct Sass_Options* options)
  {
    // Deallocate custom functions
    if (options->c_functions) {
      struct Sass_C_Function_Descriptor** this_func_data = options->c_functions;
      while ((this_func_data) && (*this_func_data)) {
        free((*this_func_data));
        ++this_func_data;
      }
    }
    // Deallocate inc paths
    if (options->include_paths) {
      struct string_list* cur;
      struct string_list* next;
      cur = options->include_paths;
      while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
      }
    }
    // Free the list container
    free(options->c_functions);
    // Make it null terminated
    options->c_functions = NULL;
  }

  // helper function, not exported, only accessible locally
  // sass_free_context is also defined in old sass_interface
  static void sass_clear_context (struct Sass_Context* ctx)
  {
    if (ctx->output_string)     free(ctx->output_string);
    if (ctx->source_map_string) free(ctx->source_map_string);
    if (ctx->error_message)     free(ctx->error_message);
    if (ctx->error_json)        free(ctx->error_json);
    free_string_array(ctx->included_files);
    sass_clear_options(ctx);
  }

  // Deallocate all associated memory with contexts
  void sass_delete_file_context (struct Sass_File_Context* ctx) { sass_clear_context(ctx); free(ctx); }
  void sass_delete_data_context (struct Sass_Data_Context* ctx) { sass_clear_context(ctx); free(ctx); }

  // Getters for sass context from specific implementations
  struct Sass_Context* sass_file_context_get_context(struct Sass_File_Context* ctx) { return ctx; }
  struct Sass_Context* sass_data_context_get_context(struct Sass_Data_Context* ctx) { return ctx; }

  // Getters for context options from Sass_Context
  struct Sass_Options* sass_context_get_options(struct Sass_Context* ctx) { return ctx; }
  struct Sass_Options* sass_file_context_get_options(struct Sass_File_Context* ctx) { return ctx; }
  struct Sass_Options* sass_data_context_get_options(struct Sass_Data_Context* ctx) { return ctx; }
  void sass_file_context_set_options (struct Sass_File_Context* ctx, struct Sass_Options* opt) { (Sass_Options) *ctx = *opt; }
  void sass_data_context_set_options (struct Sass_Data_Context* ctx, struct Sass_Options* opt) { (Sass_Options) *ctx = *opt; }

  // Create getter and setters for options
  IMPLEMENT_SASS_OPTION_ACCESSOR(int, precision);
  IMPLEMENT_SASS_OPTION_ACCESSOR(enum Sass_Output_Style, output_style);
  IMPLEMENT_SASS_OPTION_ACCESSOR(bool, source_comments);
  IMPLEMENT_SASS_OPTION_ACCESSOR(bool, source_map_embed);
  IMPLEMENT_SASS_OPTION_ACCESSOR(bool, source_map_contents);
  IMPLEMENT_SASS_OPTION_ACCESSOR(bool, omit_source_map_url);
  IMPLEMENT_SASS_OPTION_ACCESSOR(bool, is_indented_syntax_src);
  IMPLEMENT_SASS_OPTION_ACCESSOR(const char*, input_path);
  IMPLEMENT_SASS_OPTION_ACCESSOR(const char*, output_path);
  IMPLEMENT_SASS_OPTION_ACCESSOR(const char*, image_path);
  IMPLEMENT_SASS_OPTION_ACCESSOR(const char*, include_path);
  IMPLEMENT_SASS_OPTION_ACCESSOR(const char*, source_map_file);
  IMPLEMENT_SASS_OPTION_ACCESSOR(Sass_C_Function_List, c_functions);
  IMPLEMENT_SASS_OPTION_ACCESSOR(Sass_C_Import_Callback, importer);

  // Create getter and setters for context
  IMPLEMENT_SASS_CONTEXT_GETTER(int, error_status);
  IMPLEMENT_SASS_CONTEXT_GETTER(const char*, error_json);
  IMPLEMENT_SASS_CONTEXT_GETTER(const char*, error_message);
  IMPLEMENT_SASS_CONTEXT_GETTER(const char*, output_string);
  IMPLEMENT_SASS_CONTEXT_GETTER(const char*, source_map_string);
  IMPLEMENT_SASS_CONTEXT_GETTER(char**, included_files);

  // Create getter and setters for specialized contexts
  IMPLEMENT_SASS_DATA_CONTEXT_SETTER(char*, source_string);

  // Push function for include paths (no manipulation support for now)
  void sass_option_push_include_path(struct Sass_Options* options, const char* path)
  {

    struct string_list* include_paths = (struct string_list*) calloc(1, sizeof(struct string_list));
    include_paths->string = path;
    struct string_list* last = options->include_paths;
    if (!options->include_paths) {
      options->include_paths = include_paths;
    } else {
      while (last->next)
        last = last->next;
      last->next = include_paths;
    }

  }
}
