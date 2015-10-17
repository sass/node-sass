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
      Data_Context cpp_ctx(
        (Sass_Data_Context*) 0
      );
      if (c_ctx->c_functions) {
        Sass_Function_List this_func_data = c_ctx->c_functions;
        while ((this_func_data) && (*this_func_data)) {
          cpp_ctx.c_functions.push_back(*this_func_data);
          ++this_func_data;
        }
      }
      Block* root = cpp_ctx.parse();
      c_ctx->output_string = cpp_ctx.render(root);
      c_ctx->source_map_string = cpp_ctx.render_srcmap();
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;

      if (copy_strings(cpp_ctx.get_included_files(true), &c_ctx->included_files, 1) == NULL)
        throw(std::bad_alloc());
    }
    catch (Exception::InvalidSass& e) {
      std::stringstream msg_stream;
      msg_stream << e.pstate.path << ":" << e.pstate.line << ": " << e.what() << std::endl;
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
      File_Context cpp_ctx(
        (Sass_File_Context*) 0
      );
      if (c_ctx->c_functions) {
        Sass_Function_List this_func_data = c_ctx->c_functions;
        while ((this_func_data) && (*this_func_data)) {
          cpp_ctx.c_functions.push_back(*this_func_data);
          ++this_func_data;
        }
      }
      Block* root = cpp_ctx.parse();
      c_ctx->output_string = cpp_ctx.render(root);
      c_ctx->source_map_string = cpp_ctx.render_srcmap();
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;

      if (copy_strings(cpp_ctx.get_included_files(false), &c_ctx->included_files) == NULL)
        throw(std::bad_alloc());
    }
    catch (Exception::InvalidSass& e) {
      std::stringstream msg_stream;
      msg_stream << e.pstate.path << ":" << e.pstate.line << ": " << e.what() << std::endl;
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
