#include <string>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

#include "util.hpp"
#include "context.hpp"
#include "inspect.hpp"
#include "error_handling.hpp"
#include "sass/base.h"
#include "sass/interface.h"

#define LFEED "\n"

extern "C" {

  sass_context* sass_new_context()
  { return (sass_context*) calloc(1, sizeof(sass_context)); }

  void sass_free_context(sass_context* ctx)
  {
    if (ctx->output_string)     free(ctx->output_string);
    if (ctx->source_map_string) free(ctx->source_map_string);
    if (ctx->error_message)     free(ctx->error_message);
    if (ctx->c_functions)       free(ctx->c_functions);

    Sass::free_string_array(ctx->included_files);

    free(ctx);
  }

  sass_file_context* sass_new_file_context()
  { return (sass_file_context*) calloc(1, sizeof(sass_file_context)); }

  void sass_free_file_context(sass_file_context* ctx)
  {
    if (ctx->output_string)     free(ctx->output_string);
    if (ctx->source_map_string) free(ctx->source_map_string);
    if (ctx->error_message)     free(ctx->error_message);
    if (ctx->c_functions)       free(ctx->c_functions);

    Sass::free_string_array(ctx->included_files);

    free(ctx);
  }

  sass_folder_context* sass_new_folder_context()
  { return (sass_folder_context*) calloc(1, sizeof(sass_folder_context)); }

  void sass_free_folder_context(sass_folder_context* ctx)
  {
    Sass::free_string_array(ctx->included_files);
    free(ctx);
  }

  int sass_compile(sass_context* c_ctx)
  {
    using namespace Sass;
    try {
      std::string input_path = safe_str(c_ctx->input_path);
      int lastindex = static_cast<int>(input_path.find_last_of("."));
      std::string output_path;
      if (!c_ctx->output_path) {
        if (input_path != "") {
          output_path = (lastindex > -1 ? input_path.substr(0, lastindex) : input_path) + ".css";
        }
      }
      else {
          output_path = c_ctx->output_path;
      }
      Context cpp_ctx(
        Context::Data().source_c_str(c_ctx->source_string)
                       .output_path(output_path)
                       .output_style((Output_Style) c_ctx->options.output_style)
                       .is_indented_syntax_src(c_ctx->options.is_indented_syntax_src)
                       .source_comments(c_ctx->options.source_comments)
                       .source_map_file(safe_str(c_ctx->options.source_map_file))
                       .source_map_root(safe_str(c_ctx->options.source_map_root))
                       .source_map_embed(c_ctx->options.source_map_embed)
                       .source_map_contents(c_ctx->options.source_map_contents)
                       .omit_source_map_url(c_ctx->options.omit_source_map_url)
                       .include_paths_c_str(c_ctx->options.include_paths)
                       .plugin_paths_c_str(c_ctx->options.plugin_paths)
                       // .include_paths_array(0)
                       // .plugin_paths_array(0)
                       .include_paths(std::vector<std::string>())
                       .plugin_paths(std::vector<std::string>())
                       .precision(c_ctx->options.precision ? c_ctx->options.precision : 5)
                       .indent(c_ctx->options.indent ? c_ctx->options.indent : "  ")
                       .linefeed(c_ctx->options.linefeed ? c_ctx->options.linefeed : LFEED)
      );
      if (c_ctx->c_functions) {
        Sass_Function_List this_func_data = c_ctx->c_functions;
        while ((this_func_data) && (*this_func_data)) {
          cpp_ctx.c_functions.push_back(*this_func_data);
          ++this_func_data;
        }
      }
      c_ctx->output_string = cpp_ctx.compile_string();
      c_ctx->source_map_string = cpp_ctx.generate_source_map();
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;

      if (copy_strings(cpp_ctx.get_included_files(true), &c_ctx->included_files, 1) == NULL)
        throw(std::bad_alloc());
    }
    catch (Error_Invalid& e) {
      std::stringstream msg_stream;
      msg_stream << e.pstate.path << ":" << e.pstate.line << ": " << e.message << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch(std::bad_alloc& ba) {
      std::stringstream msg_stream;
      msg_stream << "Unable to allocate memory: " << ba.what() << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (std::exception& e) {
      std::stringstream msg_stream;
      msg_stream << "Error: " << e.what() << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (std::string& e) {
      std::stringstream msg_stream;
      msg_stream << "Error: " << e << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (...) {
      // couldn't find the specified file in the include paths; report an error
      std::stringstream msg_stream;
      msg_stream << "Unknown error occurred" << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    return 0;
  }

  int sass_compile_file(sass_file_context* c_ctx)
  {
    using namespace Sass;
    try {
      std::string input_path = safe_str(c_ctx->input_path);
      int lastindex = static_cast<int>(input_path.find_last_of("."));
      std::string output_path;
      if (!c_ctx->output_path) {
          output_path = (lastindex > -1 ? input_path.substr(0, lastindex) : input_path) + ".css";
      }
      else {
          output_path = c_ctx->output_path;
      }
      Context cpp_ctx(
        Context::Data().entry_point(input_path)
                       .output_path(output_path)
                       .output_style((Output_Style) c_ctx->options.output_style)
                       .is_indented_syntax_src(c_ctx->options.is_indented_syntax_src)
                       .source_comments(c_ctx->options.source_comments)
                       .source_map_file(safe_str(c_ctx->options.source_map_file))
                       .source_map_root(safe_str(c_ctx->options.source_map_root))
                       .source_map_embed(c_ctx->options.source_map_embed)
                       .source_map_contents(c_ctx->options.source_map_contents)
                       .omit_source_map_url(c_ctx->options.omit_source_map_url)
                       .include_paths_c_str(c_ctx->options.include_paths)
                       .plugin_paths_c_str(c_ctx->options.plugin_paths)
                       // .include_paths_array(0)
                       // .plugin_paths_array(0)
                       .include_paths(std::vector<std::string>())
                       .plugin_paths(std::vector<std::string>())
                       .precision(c_ctx->options.precision ? c_ctx->options.precision : 5)
                       .indent(c_ctx->options.indent ? c_ctx->options.indent : "  ")
                       .linefeed(c_ctx->options.linefeed ? c_ctx->options.linefeed : LFEED)
      );
      if (c_ctx->c_functions) {
        Sass_Function_List this_func_data = c_ctx->c_functions;
        while ((this_func_data) && (*this_func_data)) {
          cpp_ctx.c_functions.push_back(*this_func_data);
          ++this_func_data;
        }
      }
      c_ctx->output_string = cpp_ctx.compile_file();
      c_ctx->source_map_string = cpp_ctx.generate_source_map();
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;

      if (copy_strings(cpp_ctx.get_included_files(false), &c_ctx->included_files) == NULL)
        throw(std::bad_alloc());
    }
    catch (Error_Invalid& e) {
      std::stringstream msg_stream;
      msg_stream << e.pstate.path << ":" << e.pstate.line << ": " << e.message << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch(std::bad_alloc& ba) {
      std::stringstream msg_stream;
      msg_stream << "Unable to allocate memory: " << ba.what() << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (std::exception& e) {
      std::stringstream msg_stream;
      msg_stream << "Error: " << e.what() << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (std::string& e) {
      std::stringstream msg_stream;
      msg_stream << "Error: " << e << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    catch (...) {
      // couldn't find the specified file in the include paths; report an error
      std::stringstream msg_stream;
      msg_stream << "Unknown error occurred" << std::endl;
      c_ctx->error_message = sass_strdup(msg_stream.str().c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->source_map_string = 0;
    }
    return 0;
  }

  int sass_compile_folder(sass_folder_context* c_ctx)
  {
    return 1;
  }

}
