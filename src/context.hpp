#define SASS_CONTEXT

#include <string>
#include <vector>

#ifndef SASS_MEMORY_MANAGER
#include "memory_manager.hpp"
#endif

namespace Sass {
  using std::string;
  using std::vector;

  struct Context {
    class AST_Node;
    Memory_Manager<AST_Node*>& mem;
    vector<const char*> source_strs; // c-strings containing Sass file contents
    vector<string> include_paths;
    char* image_path; // for the image-url Sass function

    void collect_include_paths(const char* paths_str);
    void collect_include_paths(const char* paths_array[]);
    ~Context();
  };

}
