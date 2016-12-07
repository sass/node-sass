#ifndef SASS_SASS_FUNCTIONS_H
#define SASS_SASS_FUNCTIONS_H

#include "sass.h"

// Struct to hold custom function callback
struct Sass_Function {
  const char*      signature;
  Sass_Function_Fn function;
  void*            cookie;
};

// External import entry
struct Sass_Import {
  char* imp_path; // path as found in the import statement
  char *abs_path; // path after importer has resolved it
  char* source;
  char* srcmap;
  // error handling
  char* error;
  size_t line;
  size_t column;
};

// External call entry
struct Sass_Callee {
  const char* name;
  const char* path;
  size_t line;
  size_t column;
  enum Sass_Callee_Type type;
};

// Struct to hold importer callback
struct Sass_Importer {
  Sass_Importer_Fn importer;
  double           priority;
  void*            cookie;
};

#endif