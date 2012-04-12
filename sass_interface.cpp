#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include "document.hpp"
#include "eval_apply.hpp"
#include "sass_interface.h"

using namespace std;

extern "C" sass_context* sass_new_context()
{ return (sass_context*) malloc(sizeof(sass_context)); }

extern "C" char* sass_compile(sass_context* c_ctx)
{
  using namespace Sass;
  // TO DO: CATCH ALL EXCEPTIONS
  Context cpp_ctx;
  
  cpp_ctx.sass_path = string(c_ctx->sass_path ? c_ctx->sass_path : "");
  cpp_ctx.css_path  = string(c_ctx->css_path  ? c_ctx->css_path  : "");

  const size_t wd_len = 1024;
  char wd[wd_len];
  cpp_ctx.include_paths.push_back(getcwd(wd, wd_len));
  if (*cpp_ctx.include_paths.back().rbegin() != '/') cpp_ctx.include_paths.back() += '/';

  if (c_ctx->include_paths) {
    const char* beg = c_ctx->include_paths;
    const char* end = Prelexer::find_first<':'>(beg);
  
    while (end) {
      string path(beg, end - beg);
      if (!path.empty()) {
        if (*path.rbegin() != '/') path += '/';
        cpp_ctx.include_paths.push_back(path);
      }
      beg = end + 1;
      end = Prelexer::find_first<':'>(beg);
    }
    
    string path(beg);
    if (!path.empty()) {
      if (*path.rbegin() != '/') path += '/';
      cpp_ctx.include_paths.push_back(path);
    }
    for (int i = 0; i < cpp_ctx.include_paths.size(); ++i) {
      cerr << cpp_ctx.include_paths[i] << endl;
    }
  }
  
  Document doc(c_ctx->input_file, c_ctx->input_string, cpp_ctx);
  doc.parse_scss();
  eval(doc.root, doc.context.global_env, doc.context.function_env);
  string output(doc.emit_css(static_cast<Document::CSS_Style>(c_ctx->output_style)));
  
  char* c_output = (char*) malloc(output.size() + 1);
  strcpy(c_output, output.c_str());
  return c_output;
}

// 
//   This is when you want to compile a whole folder of stuff
// 
// var opts = sass_new_context();
// opts->sassPath = "/Users/hcatlin/dev/asset/stylesheet";
// opts->cssPath = "/Users/hcatlin/dev/asset/stylesheets/.css";
// opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
// opts->outputStyle => SASS_STYLE_COMPRESSED;
// sass_compile(opts, &callbackfunction);
// 
// 
//   This is when you want to compile a string
// 
// opts = sass_new_context();
// opts->inputString = "a { width: 50px; }";
// opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
// opts->outputStyle => SASS_STYLE_EXPANDED;
// var cssResult = sass_compile(opts, &callbackfunction);