#define SASS_CONTEXT

#include <string>
#include <vector>
#include <map>
#include "kwd_arg_macros.hpp"

#ifndef SASS_MEMORY_MANAGER
#include "memory_manager.hpp"
#endif

#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
#endif

#ifndef SASS_SOURCE_MAP
#include "source_map.hpp"
#endif

#ifndef SASS_SUBSET_MAP
#include "subset_map.hpp"
#endif

#define BUFFERSIZE 255
#include "b64/encode.h"

#include "sass_functions.h"

struct Sass_C_Function_Descriptor;

namespace Sass {
  using namespace std;
  class AST_Node;
  class Block;
  class Expression;
  class Color;
  struct Backtrace;
  // typedef const char* Signature;
  // struct Context;
  // typedef Environment<AST_Node*> Env;
  // typedef Expression* (*Native_Function)(Env&, Context&, Signature, string, size_t);

  enum Output_Style { NESTED, EXPANDED, COMPACT, COMPRESSED, FORMATTED };

  struct Sass_Queued {
    string abs_path;
    string load_path;
    const char* source;
  public:
    Sass_Queued(const string& load_path, const string& abs_path, const char* source);
  };

  struct Context {
    Memory_Manager<AST_Node> mem;

    const char* source_c_str;

    // c-strs containing Sass file contents
    // we will overtake ownership of memory
    vector<const char*> sources;
    // absolute paths to includes
    vector<string> included_files;
    // relative links to includes
    vector<string> include_links;
    // vectors above have same size

    vector<string> include_paths; // lookup paths for includes
    vector<Sass_Queued> queue; // queue of files to be parsed
    map<string, Block*> style_sheets; // map of paths to ASTs
    SourceMap source_map;
    vector<Sass_C_Function_Callback> c_functions;

    string       image_path; // for the image-url Sass function
    string       input_path; // for relative paths in src-map
    string       output_path; // for relative paths to the output
    bool         source_comments; // for inline debug comments in css output
    Output_Style output_style; // output style for the generated css code
    string       source_map_file; // path to source map file (enables feature)
    bool         source_map_embed; // embed in sourceMappingUrl (as data-url)
    bool         source_map_contents; // insert included contents into source map
    bool         omit_source_map_url; // disable source map comment in css output
    bool         is_indented_syntax_src; // treat source string as sass

    // overload import calls
    Sass_C_Import_Callback importer;
    vector<struct Sass_Import*> import_stack;

    map<string, Color*> names_to_colors;
    map<int, string>    colors_to_names;

    size_t precision; // precision for outputting fractional numbers
    bool _skip_source_map_update; // status flag to skip source map updates

    KWD_ARG_SET(Data) {
      KWD_ARG(Data, const char*,     source_c_str);
      KWD_ARG(Data, string,          entry_point);
      KWD_ARG(Data, string,          input_path);
      KWD_ARG(Data, string,          output_path);
      KWD_ARG(Data, string,          image_path);
      KWD_ARG(Data, const char*,     include_paths_c_str);
      KWD_ARG(Data, const char**,    include_paths_array);
      KWD_ARG(Data, vector<string>,  include_paths);
      KWD_ARG(Data, bool,            source_comments);
      KWD_ARG(Data, Output_Style,    output_style);
      KWD_ARG(Data, string,          source_map_file);
      KWD_ARG(Data, bool,            omit_source_map_url);
      KWD_ARG(Data, bool,            is_indented_syntax_src);
      KWD_ARG(Data, size_t,          precision);
      KWD_ARG(Data, bool,            _skip_source_map_update);
      KWD_ARG(Data, bool,            source_map_embed);
      KWD_ARG(Data, bool,            source_map_contents);
      KWD_ARG(Data, Sass_C_Import_Callback, importer);
    };

    Context(Data);
    ~Context();
    static string get_cwd();
    void setup_color_map();
    string add_file(string);
    Block* parse_file();
    string add_file(string, string);
    Block* parse_string();
    void add_source(string, string, const char*);
    // allow to optionally overwrite the input path
    // default argument for input_path is string("stdin")
    // usefull to influence the source-map generating etc.
    char* compile_file();
    char* compile_string();
    char* compile_block(Block* root);
    char* generate_source_map();

    vector<string> get_included_files(size_t skip = 0);

  private:
    void collect_include_paths(const char* paths_str);
    void collect_include_paths(const char** paths_array);
    string format_source_mapping_url(const string& file);

    string cwd;

    // void register_built_in_functions(Env* env);
    // void register_function(Signature sig, Native_Function f, Env* env);
    // void register_function(Signature sig, Native_Function f, size_t arity, Env* env);
    // void register_overload_stub(string name, Env* env);

  public:
    Subset_Map<string, pair<Complex_Selector*, Compound_Selector*> > subset_map;
  };

}
