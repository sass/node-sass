#ifndef SASS_CONTEXT_H
#define SASS_CONTEXT_H

#include <string>
#include <vector>
#include <map>

#define BUFFERSIZE 255
#include "b64/encode.h"

#include "ast_fwd_decl.hpp"
#include "kwd_arg_macros.hpp"
#include "memory_manager.hpp"
#include "environment.hpp"
#include "source_map.hpp"
#include "subset_map.hpp"
#include "output.hpp"
#include "plugins.hpp"
#include "file.hpp"
#include "sass.h"

struct Sass_Function;

namespace Sass {

  class Context {
  public:
    size_t head_imports;
    Memory_Manager mem;

    struct Sass_Options* c_options;
    struct Sass_Compiler* c_compiler;
    char* source_c_str;

    // c-strs containing Sass file contents
    // we will overtake ownership of memory
    std::vector<char*> sources;
    // strings get freed with context
    std::vector<char*> strings;
    // absolute paths to includes
    std::vector<std::string> included_files;
    // relative links to includes
    std::vector<std::string> include_links;
    // vectors above have same size

    std::vector<std::string> plugin_paths; // relative paths to load plugins
    std::vector<std::string> include_paths; // lookup paths for includes
    std::vector<Sass_Queued> queue; // queue of files to be parsed
    std::map<std::string, Block*> style_sheets; // map of paths to ASTs
    // SourceMap source_map;
    Output emitter;

    std::vector<Sass_Importer_Entry> c_headers;
    std::vector<Sass_Importer_Entry> c_importers;
    std::vector<Sass_Function_Entry> c_functions;

    void add_c_header(Sass_Importer_Entry header);
    void add_c_importer(Sass_Importer_Entry importer);
    void add_c_function(Sass_Function_Entry function);

    std::string       indent; // String to be used for indentation
    std::string       linefeed; // String to be used for line feeds
    std::string       input_path; // for relative paths in src-map
    std::string       output_path; // for relative paths to the output
    bool         source_comments; // for inline debug comments in css output
    Output_Style output_style; // output style for the generated css code
    std::string       source_map_file; // path to source map file (enables feature)
    std::string       source_map_root; // path for sourceRoot property (pass-through)
    bool         source_map_embed; // embed in sourceMappingUrl (as data-url)
    bool         source_map_contents; // insert included contents into source map
    bool         omit_source_map_url; // disable source map comment in css output
    bool         is_indented_syntax_src; // treat source string as sass

    // overload import calls
    std::vector<Sass_Import_Entry> import_stack;

    size_t precision; // precision for outputting fractional numbers

    KWD_ARG_SET(Data) {
      KWD_ARG(Data, struct Sass_Options*,      c_options)
      KWD_ARG(Data, struct Sass_Compiler*,     c_compiler)
      KWD_ARG(Data, char*,                     source_c_str)
      KWD_ARG(Data, std::string,               entry_point)
      KWD_ARG(Data, std::string,               input_path)
      KWD_ARG(Data, std::string,               output_path)
      KWD_ARG(Data, std::string,               indent)
      KWD_ARG(Data, std::string,               linefeed)
      KWD_ARG(Data, const char*,               include_paths_c_str)
      KWD_ARG(Data, const char*,               plugin_paths_c_str)
      // KWD_ARG(Data, const char**,              include_paths_array)
      // KWD_ARG(Data, const char**,              plugin_paths_array)
      KWD_ARG(Data, std::vector<std::string>,  include_paths)
      KWD_ARG(Data, std::vector<std::string>,  plugin_paths)
      KWD_ARG(Data, bool,                      source_comments)
      KWD_ARG(Data, Output_Style,              output_style)
      KWD_ARG(Data, std::string,               source_map_file)
      KWD_ARG(Data, std::string,               source_map_root)
      KWD_ARG(Data, bool,                      omit_source_map_url)
      KWD_ARG(Data, bool,                      is_indented_syntax_src)
      KWD_ARG(Data, size_t,                    precision)
      KWD_ARG(Data, bool,                      source_map_embed)
      KWD_ARG(Data, bool,                      source_map_contents)
    };

    Context(Data);
    ~Context();
    static std::string get_cwd();

    Block* parse_file();
    Block* parse_string();
    void add_source(std::string, std::string, char*);

    std::string add_file(const std::string& imp_path, bool delay = false);
    std::string add_file(const std::string& imp_path, const std::string& abs_path, ParserState pstate);

    void process_queue_entry(Sass_Queued& entry, size_t idx);

    // allow to optionally overwrite the input path
    // default argument for input_path is std::string("stdin")
    // usefull to influence the source-map generating etc.
    char* compile_file();
    char* compile_string();
    char* compile_block(Block* root);
    char* generate_source_map();

    std::vector<std::string> get_included_files(bool skip = false, size_t headers = 0);

  private:
    void collect_plugin_paths(const char* paths_str);
    void collect_plugin_paths(const char** paths_array);
    void collect_include_paths(const char* paths_str);
    void collect_include_paths(const char** paths_array);
    std::string format_embedded_source_map();
    std::string format_source_mapping_url(const std::string& out_path);

    std::string cwd;
    Plugins plugins;

    // void register_built_in_functions(Env* env);
    // void register_function(Signature sig, Native_Function f, Env* env);
    // void register_function(Signature sig, Native_Function f, size_t arity, Env* env);
    // void register_overload_stub(std::string name, Env* env);

  public:
    Subset_Map<std::string, std::pair<Complex_Selector*, Compound_Selector*> > subset_map;
  };

}

#endif
