#include <iostream>
#include <sstream>
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
    cerr << "PARSED" << endl;
    eval(doc.root, doc.context.global_env, doc.context.function_env, doc.context.registry);
    cerr << "EVALUATED" << endl;
    string output(doc.emit_css(static_cast<Document::CSS_Style>(style)));
    cerr << "EMITTED" << endl;
    
    cerr << "Allocations:\t" << Node::allocations << endl;
    cerr << "Registry size:\t" << doc.context.registry.size() << endl;
    
    int i;
    for (i = 0; i < doc.context.registry.size(); ++i) {
      delete doc.context.registry[i];
    }
    cerr << "Deallocations:\t" << i << endl;
    
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
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;
    }
    catch (Error& e) {
      stringstream msg_stream;
      msg_stream << "ERROR -- " << e.file_name << ", line " << e.line_number << ": " << e.message << endl;
      string msg(msg_stream.str());
      char* msg_str = (char*) malloc(msg.size() + 1);
      strcpy(msg_str, msg.c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->error_message = msg_str;
    }
    catch(bad_alloc& ba) {
      stringstream msg_stream;
      msg_stream << "ERROR -- unable to allocate memory: " << ba.what() << endl;
      string msg(msg_stream.str());
      char* msg_str = (char*) malloc(msg.size() + 1);
      strcpy(msg_str, msg.c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->error_message = msg_str;
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
      cerr << "MADE A DOC AND CONTEXT OBJ" << endl;
      cerr << "REGISTRY: " << doc.context.registry.size() << endl;
      c_ctx->output_string = process_document(doc, c_ctx->options.output_style);
      c_ctx->error_message = 0;
      c_ctx->error_status = 0;
    }
    catch (Error& e) {
      stringstream msg_stream;
      msg_stream << "ERROR -- " << e.file_name << ", line " << e.line_number << ": " << e.message << endl;
      string msg(msg_stream.str());
      char* msg_str = (char*) malloc(msg.size() + 1);
      strcpy(msg_str, msg.c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->error_message = msg_str;
    }
    catch(bad_alloc& ba) {
      stringstream msg_stream;
      msg_stream << "ERROR -- unable to allocate memory: " << ba.what() << endl;
      string msg(msg_stream.str());
      char* msg_str = (char*) malloc(msg.size() + 1);
      strcpy(msg_str, msg.c_str());
      c_ctx->error_status = 1;
      c_ctx->output_string = 0;
      c_ctx->error_message = msg_str;
    }
    // TO DO: CATCH EVERYTHING ELSE
    return 0;
  }

}