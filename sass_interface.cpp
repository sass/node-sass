#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include "document.hpp"
#include "eval_apply.hpp"
#include "error.hpp"
#include "sass_interface.h"

extern "C" {

  using namespace std;

  sass_context* sass_new_context()
  { return (sass_context*) malloc(sizeof(sass_context)); }
  
  void sass_free_context(sass_context* ctx)
  { 
    free(ctx->output_string);
    free(ctx);
  }

  sass_file_context* sass_new_file_context()
  { return (sass_file_context*) malloc(sizeof(sass_file_context)); }
  
  void sass_free_file_context(sass_file_context* ctx)
  { 
    free(ctx->output_string);
    free(ctx);
  }
  
  sass_folder_context* sass_new_folder_context()
  { return (sass_folder_context*) malloc(sizeof(sass_folder_context)); }
  
  static char* process_document(Sass::Document& doc, int style)
  {
    using namespace Sass;
    
    doc.parse_scss();
    eval(doc.root, doc.context.global_env, doc.context.function_env);
    string output(doc.emit_css(static_cast<Document::CSS_Style>(style)));
    
    char* c_output = (char*) malloc(output.size() + 1);
    strcpy(c_output, output.c_str());
    return c_output;
  }

  int sass_compile(sass_context* c_ctx)
  {
    using namespace Sass;
    try {
      Context cpp_ctx(c_ctx->options.include_paths);
      Document doc(0, c_ctx->input_string, cpp_ctx);
      c_ctx->output_string = process_document(doc, c_ctx->options.output_style);
    }
    catch (Error& e) {
      cerr << "ERROR -- " << e.file_name << ", line " << e.line_number << ": " << e.message << endl;
      c_ctx->output_string = 0;
    }
    catch(bad_alloc& ba) {
      cerr << "ERROR -- unable to allocate memory: " << ba.what() << endl;
    }
    // TO DO: CATCH EVERYTHING ELSE
    return 0;
  }
  
  int sass_compile_file(sass_file_context* c_ctx)
  {
    using namespace Sass;
    try {
      Context cpp_ctx(c_ctx->options.include_paths);
      Document doc(c_ctx->input_path, 0, cpp_ctx);
      c_ctx->output_string = process_document(doc, c_ctx->options.output_style);
    }
    catch (Error& e) {
      cerr << "ERROR -- " << e.file_name << ", line " << e.line_number << ": " << e.message << endl;
      c_ctx->output_string = 0;
    }
    catch(bad_alloc& ba) {
      cerr << "ERROR -- unable to allocate memory: " << ba.what() << endl;
    }
    // TO DO: CATCH EVERYTHING ELSE
    return 0;
  }

}