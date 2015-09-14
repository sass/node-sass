#ifdef _WIN32
#define PATH_SEP ';'
#else
#define PATH_SEP ':'
#endif

#include <string>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <iostream>

#include "ast.hpp"
#include "util.hpp"
#include "sass.h"
#include "context.hpp"
#include "plugins.hpp"
#include "constants.hpp"
#include "parser.hpp"
#include "file.hpp"
#include "inspect.hpp"
#include "output.hpp"
#include "expand.hpp"
#include "eval.hpp"
#include "cssize.hpp"
#include "listize.hpp"
#include "extend.hpp"
#include "remove_placeholders.hpp"
#include "functions.hpp"
#include "backtrace.hpp"
#include "sass2scss.h"
#include "prelexer.hpp"
#include "emitter.hpp"

namespace Sass {
  using namespace Constants;
  using namespace File;
  using namespace Sass;

  Sass_Queued::Sass_Queued(const std::string& load_path, const std::string& abs_path, const char* source)
  {
    this->load_path = load_path;
    this->abs_path = abs_path;
    this->source = source;
  }

  inline bool sort_importers (const Sass_Importer_Entry& i, const Sass_Importer_Entry& j)
  { return sass_importer_get_priority(i) > sass_importer_get_priority(j); }

  Context::Context(Context::Data initializers)
  : // Output(this),
    head_imports(0),
    mem(Memory_Manager()),
    c_options               (initializers.c_options()),
    c_compiler              (initializers.c_compiler()),
    source_c_str            (initializers.source_c_str()),
    sources                 (std::vector<char*>()),
    strings                 (std::vector<char*>()),
    plugin_paths            (initializers.plugin_paths()),
    include_paths           (initializers.include_paths()),
    queue                   (std::vector<Sass_Queued>()),
    style_sheets            (std::map<std::string, Block*>()),
    emitter (this),
    c_headers               (std::vector<Sass_Importer_Entry>()),
    c_importers             (std::vector<Sass_Importer_Entry>()),
    c_functions             (std::vector<Sass_Function_Entry>()),
    indent                  (initializers.indent()),
    linefeed                (initializers.linefeed()),
    input_path              (make_canonical_path(initializers.input_path())),
    output_path             (make_canonical_path(initializers.output_path())),
    source_comments         (initializers.source_comments()),
    output_style            (initializers.output_style()),
    source_map_file         (make_canonical_path(initializers.source_map_file())),
    source_map_root         (initializers.source_map_root()), // pass-through
    source_map_embed        (initializers.source_map_embed()),
    source_map_contents     (initializers.source_map_contents()),
    omit_source_map_url     (initializers.omit_source_map_url()),
    is_indented_syntax_src  (initializers.is_indented_syntax_src()),
    precision               (initializers.precision()),
    plugins(),
    subset_map              (Subset_Map<std::string, std::pair<Complex_Selector*, Compound_Selector*> >())
  {

    cwd = get_cwd();

    // enforce some safe defaults
    // used to create relative file links
    if (input_path == "") input_path = "stdin";
    if (output_path == "") output_path = "stdout";

    include_paths.push_back(cwd);
    collect_include_paths(initializers.include_paths_c_str());
    // collect_include_paths(initializers.include_paths_array());
    collect_plugin_paths(initializers.plugin_paths_c_str());
    // collect_plugin_paths(initializers.plugin_paths_array());

    for (size_t i = 0, S = plugin_paths.size(); i < S; ++i) {
      plugins.load_plugins(plugin_paths[i]);
    }

    for(auto fn : plugins.get_functions()) {
      c_functions.push_back(fn);
    }
    for(auto fn : plugins.get_headers()) {
      c_headers.push_back(fn);
    }
    for(auto fn : plugins.get_importers()) {
      c_importers.push_back(fn);
    }

    sort (c_headers.begin(), c_headers.end(), sort_importers);
    sort (c_importers.begin(), c_importers.end(), sort_importers);
    std::string entry_point = initializers.entry_point();
    if (!entry_point.empty()) {
      std::string result(add_file(entry_point, true));
      if (result.empty()) {
        throw "File to read not found or unreadable: " + entry_point;
      }
    }

    emitter.set_filename(resolve_relative_path(output_path, source_map_file, cwd));

  }

  void Context::add_c_function(Sass_Function_Entry function)
  {
    c_functions.push_back(function);
  }
  void Context::add_c_header(Sass_Importer_Entry header)
  {
    c_headers.push_back(header);
    // need to sort the array afterwards (no big deal)
    sort (c_headers.begin(), c_headers.end(), sort_importers);
  }
  void Context::add_c_importer(Sass_Importer_Entry importer)
  {
    c_importers.push_back(importer);
    // need to sort the array afterwards (no big deal)
    sort (c_importers.begin(), c_importers.end(), sort_importers);
  }

  Context::~Context()
  {
    // make sure we free the source even if not processed!
    if (sources.size() == 0 && source_c_str) free(source_c_str);
    // sources are allocated by strdup or malloc (overtaken from C code)
    for (size_t i = 0; i < sources.size(); ++i) free(sources[i]);
    // free all strings we kept alive during compiler execution
    for (size_t n = 0; n < strings.size(); ++n) free(strings[n]);
    // everything that gets put into sources will be freed by us
    for (size_t m = 0; m < import_stack.size(); ++m) sass_delete_import(import_stack[m]);
    // clear inner structures (vectors) and input source
    sources.clear(); import_stack.clear(); source_c_str = 0;
  }

  void Context::collect_include_paths(const char* paths_str)
  {

    if (paths_str) {
      const char* beg = paths_str;
      const char* end = Prelexer::find_first<PATH_SEP>(beg);

      while (end) {
        std::string path(beg, end - beg);
        if (!path.empty()) {
          if (*path.rbegin() != '/') path += '/';
          include_paths.push_back(path);
        }
        beg = end + 1;
        end = Prelexer::find_first<PATH_SEP>(beg);
      }

      std::string path(beg);
      if (!path.empty()) {
        if (*path.rbegin() != '/') path += '/';
        include_paths.push_back(path);
      }
    }
  }

  void Context::collect_include_paths(const char** paths_array)
  {
    if (paths_array) {
      for (size_t i = 0; paths_array[i]; i++) {
        collect_include_paths(paths_array[i]);
      }
    }
  }

  void Context::collect_plugin_paths(const char* paths_str)
  {

    if (paths_str) {
      const char* beg = paths_str;
      const char* end = Prelexer::find_first<PATH_SEP>(beg);

      while (end) {
        std::string path(beg, end - beg);
        if (!path.empty()) {
          if (*path.rbegin() != '/') path += '/';
          plugin_paths.push_back(path);
        }
        beg = end + 1;
        end = Prelexer::find_first<PATH_SEP>(beg);
      }

      std::string path(beg);
      if (!path.empty()) {
        if (*path.rbegin() != '/') path += '/';
        plugin_paths.push_back(path);
      }
    }
  }

  void Context::collect_plugin_paths(const char** paths_array)
  {
    if (paths_array) {
      for (size_t i = 0; paths_array[i]; i++) {
        collect_plugin_paths(paths_array[i]);
      }
    }
  }
  void Context::add_source(std::string load_path, std::string abs_path, char* contents)
  {
    sources.push_back(contents);
    included_files.push_back(abs_path);
    queue.push_back(Sass_Queued(load_path, abs_path, contents));
    emitter.add_source_index(sources.size() - 1);
    include_links.push_back(resolve_relative_path(abs_path, source_map_file, cwd));
  }

  // Add a new import file to the context
  std::string Context::add_file(const std::string& file, bool delay)
  {
    using namespace File;
    std::string path(make_canonical_path(file));
    std::string resolved(find_file(path, include_paths));
    if (resolved == "") return resolved;
    if (char* contents = read_file(resolved)) {
      add_source(path, resolved, contents);
      style_sheets[path] = 0;
      if (delay == false) {
        size_t i = queue.size() - 1;
        process_queue_entry(queue[i], i);
      }
      return path;
    }
    return std::string("");
  }

  // Add a new import file to the context
  // This has some previous directory context
  std::string Context::add_file(const std::string& base, const std::string& file, ParserState pstate)
  {
    using namespace File;
    std::string path(make_canonical_path(file));
    std::string base_file(join_paths(base, path));
    if (style_sheets.count(base_file)) return base_file;
    std::vector<Sass_Queued> resolved(resolve_file(base, path));
    if (resolved.size() > 1) {
      std::stringstream msg_stream;
      msg_stream << "It's not clear which file to import for ";
      msg_stream << "'@import \"" << file << "\"'." << "\n";
      msg_stream << "Candidates:" << "\n";
      for (size_t i = 0, L = resolved.size(); i < L; ++i)
      { msg_stream << "  " << resolved[i].load_path << "\n"; }
      msg_stream << "Please delete or rename all but one of these files." << "\n";
      error(msg_stream.str(), pstate);
    }
    if (resolved.size()) {
      if (char* contents = read_file(resolved[0].abs_path)) {
        add_source(base_file, resolved[0].abs_path, contents);
        style_sheets[base_file] = 0;
        size_t i = queue.size() - 1;
        process_queue_entry(queue[i], i);
        return base_file;
      }
    }
    // now go the regular code path
    return add_file(path);
  }

  void register_function(Context&, Signature sig, Native_Function f, Env* env);
  void register_function(Context&, Signature sig, Native_Function f, size_t arity, Env* env);
  void register_overload_stub(Context&, std::string name, Env* env);
  void register_built_in_functions(Context&, Env* env);
  void register_c_functions(Context&, Env* env, Sass_Function_List);
  void register_c_function(Context&, Env* env, Sass_Function_Entry);

  char* Context::compile_block(Block* root)
  {
    if (!root) return 0;
    root->perform(&emitter);
    emitter.finalize();
    OutputBuffer emitted = emitter.get_buffer();
    std::string output = emitted.buffer;
    if (!omit_source_map_url) {
      if (source_map_embed) {
       output += linefeed + format_embedded_source_map();
      }
      else if (source_map_file != "") {
        output += linefeed + format_source_mapping_url(source_map_file);
      }
    }
    return sass_strdup(output.c_str());
  }

  void Context::process_queue_entry(Sass_Queued& entry, size_t i)
  {
    if (style_sheets[queue[i].load_path]) return;
    Sass_Import_Entry import = sass_make_import(
      queue[i].load_path.c_str(),
      queue[i].abs_path.c_str(),
      0, 0
    );
    import_stack.push_back(import);
    // keep a copy of the path around (for parser states)
    strings.push_back(sass_strdup(queue[i].abs_path.c_str()));
    ParserState pstate(strings.back(), queue[i].source, i);
    Parser p(Parser::from_c_str(queue[i].source, *this, pstate));
    Block* ast = p.parse();
    sass_delete_import(import_stack.back());
    import_stack.pop_back();
    // ToDo: we store by load_path, which can lead
    // to duplicates if importer reports the same path
    // Maybe we should add an error for duplicates!?
    style_sheets[queue[i].load_path] = ast;
  }

  Block* Context::parse_file()
  {
    Block* root = 0;
    for (size_t i = 0; i < queue.size(); ++i) {
      process_queue_entry(queue[i], i);
      if (i == 0) root = style_sheets[queue[i].load_path];
    }
    if (root == 0) return 0;

    Env global; // create root environment
    // register built-in functions on env
    register_built_in_functions(*this, &global);
    // register custom functions (defined via C-API)
    for (size_t i = 0, S = c_functions.size(); i < S; ++i)
    { register_c_function(*this, &global, c_functions[i]); }
    // create initial backtrace entry
    Backtrace backtrace(0, ParserState("", 0), "");
    // create crtp visitor objects
    Expand expand(*this, &global, &backtrace);
    Cssize cssize(*this, &backtrace);
    // expand and eval the tree
    root = root->perform(&expand)->block();
    // merge and bubble certain rules
    root = root->perform(&cssize)->block();
    // should we extend something?
    if (!subset_map.empty()) {
      // create crtp visitor object
      Extend extend(*this, subset_map);
      // extend tree nodes
      root->perform(&extend);
    }

    // clean up by removing empty placeholders
    // ToDo: maybe we can do this somewhere else?
    Remove_Placeholders remove_placeholders(*this);
    root->perform(&remove_placeholders);
    // return processed tree
    return root;
  }
  // EO parse_file

  Block* Context::parse_string()
  {
    if (!source_c_str) return 0;
    queue.clear();
    if(is_indented_syntax_src) {
      char * contents = sass2scss(source_c_str, SASS2SCSS_PRETTIFY_1 | SASS2SCSS_KEEP_COMMENT);
      add_source(input_path, input_path, contents);
      free(source_c_str);
      return parse_file();
    }
    add_source(input_path, input_path, source_c_str);
    size_t idx = queue.size() - 1;
    process_queue_entry(queue[idx], idx);
    return parse_file();
  }

  char* Context::compile_file()
  {
    // returns NULL if something fails
    return compile_block(parse_file());
  }

  char* Context::compile_string()
  {
    // returns NULL if something fails
    return compile_block(parse_string());
  }

  std::string Context::format_embedded_source_map()
  {
    std::string map = emitter.generate_source_map(*this);
    std::istringstream is( map );
    std::ostringstream buffer;
    base64::encoder E;
    E.encode(is, buffer);
    std::string url = "data:application/json;base64," + buffer.str();
    url.erase(url.size() - 1);
    return "/*# sourceMappingURL=" + url + " */";
  }

  std::string Context::format_source_mapping_url(const std::string& file)
  {
    std::string url = resolve_relative_path(file, output_path, cwd);
    return "/*# sourceMappingURL=" + url + " */";
  }

  char* Context::generate_source_map()
  {
    if (source_map_file == "") return 0;
    char* result = 0;
    std::string map = emitter.generate_source_map(*this);
    result = sass_strdup(map.c_str());
    return result;
  }


  // for data context we want to start after "stdin"
  // we probably always want to skip the header includes?
  std::vector<std::string> Context::get_included_files(bool skip, size_t headers)
  {
      // create a copy of the vector for manupulations
      std::vector<std::string> includes = included_files;
      if (includes.size() == 0) return includes;
      if (skip) { includes.erase( includes.begin(), includes.begin() + 1 + headers); }
      else { includes.erase( includes.begin() + 1, includes.begin() + 1 + headers); }
      includes.erase( std::unique( includes.begin(), includes.end() ), includes.end() );
      std::sort( includes.begin() + (skip ? 0 : 1), includes.end() );
      return includes;
  }

  std::string Context::get_cwd()
  {
    return Sass::File::get_cwd();
  }

  void register_function(Context& ctx, Signature sig, Native_Function f, Env* env)
  {
    Definition* def = make_native_function(sig, f, ctx);
    def->environment(env);
    (*env)[def->name() + "[f]"] = def;
  }

  void register_function(Context& ctx, Signature sig, Native_Function f, size_t arity, Env* env)
  {
    Definition* def = make_native_function(sig, f, ctx);
    std::stringstream ss;
    ss << def->name() << "[f]" << arity;
    def->environment(env);
    (*env)[ss.str()] = def;
  }

  void register_overload_stub(Context& ctx, std::string name, Env* env)
  {
    Definition* stub = SASS_MEMORY_NEW(ctx.mem, Definition,
                                       ParserState("[built-in function]"),
                                       0,
                                       name,
                                       0,
                                       0,
                                       true);
    (*env)[name + "[f]"] = stub;
  }


  void register_built_in_functions(Context& ctx, Env* env)
  {
    using namespace Functions;
    // RGB Functions
    register_function(ctx, rgb_sig, rgb, env);
    register_overload_stub(ctx, "rgba", env);
    register_function(ctx, rgba_4_sig, rgba_4, 4, env);
    register_function(ctx, rgba_2_sig, rgba_2, 2, env);
    register_function(ctx, red_sig, red, env);
    register_function(ctx, green_sig, green, env);
    register_function(ctx, blue_sig, blue, env);
    register_function(ctx, mix_sig, mix, env);
    // HSL Functions
    register_function(ctx, hsl_sig, hsl, env);
    register_function(ctx, hsla_sig, hsla, env);
    register_function(ctx, hue_sig, hue, env);
    register_function(ctx, saturation_sig, saturation, env);
    register_function(ctx, lightness_sig, lightness, env);
    register_function(ctx, adjust_hue_sig, adjust_hue, env);
    register_function(ctx, lighten_sig, lighten, env);
    register_function(ctx, darken_sig, darken, env);
    register_function(ctx, saturate_sig, saturate, env);
    register_function(ctx, desaturate_sig, desaturate, env);
    register_function(ctx, grayscale_sig, grayscale, env);
    register_function(ctx, complement_sig, complement, env);
    register_function(ctx, invert_sig, invert, env);
    // Opacity Functions
    register_function(ctx, alpha_sig, alpha, env);
    register_function(ctx, opacity_sig, alpha, env);
    register_function(ctx, opacify_sig, opacify, env);
    register_function(ctx, fade_in_sig, opacify, env);
    register_function(ctx, transparentize_sig, transparentize, env);
    register_function(ctx, fade_out_sig, transparentize, env);
    // Other Color Functions
    register_function(ctx, adjust_color_sig, adjust_color, env);
    register_function(ctx, scale_color_sig, scale_color, env);
    register_function(ctx, change_color_sig, change_color, env);
    register_function(ctx, ie_hex_str_sig, ie_hex_str, env);
    // String Functions
    register_function(ctx, unquote_sig, sass_unquote, env);
    register_function(ctx, quote_sig, sass_quote, env);
    register_function(ctx, str_length_sig, str_length, env);
    register_function(ctx, str_insert_sig, str_insert, env);
    register_function(ctx, str_index_sig, str_index, env);
    register_function(ctx, str_slice_sig, str_slice, env);
    register_function(ctx, to_upper_case_sig, to_upper_case, env);
    register_function(ctx, to_lower_case_sig, to_lower_case, env);
    // Number Functions
    register_function(ctx, percentage_sig, percentage, env);
    register_function(ctx, round_sig, round, env);
    register_function(ctx, ceil_sig, ceil, env);
    register_function(ctx, floor_sig, floor, env);
    register_function(ctx, abs_sig, abs, env);
    register_function(ctx, min_sig, min, env);
    register_function(ctx, max_sig, max, env);
    register_function(ctx, random_sig, random, env);
    // List Functions
    register_function(ctx, length_sig, length, env);
    register_function(ctx, nth_sig, nth, env);
    register_function(ctx, set_nth_sig, set_nth, env);
    register_function(ctx, index_sig, index, env);
    register_function(ctx, join_sig, join, env);
    register_function(ctx, append_sig, append, env);
    register_function(ctx, zip_sig, zip, env);
    register_function(ctx, list_separator_sig, list_separator, env);
    // Map Functions
    register_function(ctx, map_get_sig, map_get, env);
    register_function(ctx, map_merge_sig, map_merge, env);
    register_function(ctx, map_remove_sig, map_remove, env);
    register_function(ctx, map_keys_sig, map_keys, env);
    register_function(ctx, map_values_sig, map_values, env);
    register_function(ctx, map_has_key_sig, map_has_key, env);
    register_function(ctx, keywords_sig, keywords, env);
    // Introspection Functions
    register_function(ctx, type_of_sig, type_of, env);
    register_function(ctx, unit_sig, unit, env);
    register_function(ctx, unitless_sig, unitless, env);
    register_function(ctx, comparable_sig, comparable, env);
    register_function(ctx, variable_exists_sig, variable_exists, env);
    register_function(ctx, global_variable_exists_sig, global_variable_exists, env);
    register_function(ctx, function_exists_sig, function_exists, env);
    register_function(ctx, mixin_exists_sig, mixin_exists, env);
    register_function(ctx, feature_exists_sig, feature_exists, env);
    register_function(ctx, call_sig, call, env);
    // Boolean Functions
    register_function(ctx, not_sig, sass_not, env);
    register_function(ctx, if_sig, sass_if, env);
    // Misc Functions
    register_function(ctx, inspect_sig, inspect, env);
    register_function(ctx, unique_id_sig, unique_id, env);
    // Selector functions
    register_function(ctx, selector_nest_sig, selector_nest, env);
    register_function(ctx, selector_append_sig, selector_append, env);
    register_function(ctx, selector_extend_sig, selector_extend, env);
    register_function(ctx, selector_replace_sig, selector_replace, env);
    register_function(ctx, selector_unify_sig, selector_unify, env);
    register_function(ctx, is_superselector_sig, is_superselector, env);
    register_function(ctx, simple_selectors_sig, simple_selectors, env);
    register_function(ctx, selector_parse_sig, selector_parse, env);
  }

  void register_c_functions(Context& ctx, Env* env, Sass_Function_List descrs)
  {
    while (descrs && *descrs) {
      register_c_function(ctx, env, *descrs);
      ++descrs;
    }
  }
  void register_c_function(Context& ctx, Env* env, Sass_Function_Entry descr)
  {
    Definition* def = make_c_function(descr, ctx);
    def->environment(env);
    (*env)[def->name() + "[f]"] = def;
  }

}
