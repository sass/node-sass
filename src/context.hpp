#define SASS_CONTEXT

#include <string>
#include <vector>
#include <map>
#include "kwd_arg_macros.hpp"

#ifndef SASS_MEMORY_MANAGER
#include "memory_manager.hpp"
#endif

#ifndef SASS_STYLE_SHEET
#include "style_sheet.hpp"
#endif

namespace Sass {
  using namespace std;
  class AST_Node;
  class Block;

  struct Context {
    Memory_Manager<AST_Node*> mem;

    vector<const char*> sources; // c-strs containing Sass file contents
    vector<string> include_paths;
    vector<pair<string, const char*> > queue; // queue of files to be parsed
    map<string, Block*> style_sheets; // map of paths to ASTs

    string image_path; // for the image-url Sass function

    void collect_include_paths(const char* paths_str);
    void collect_include_paths(const char* paths_array[]);
    void add_file(string path);

    KWD_ARG_SET(Data) {
      KWD_ARG(Data, string,          entry_point);
      KWD_ARG(Data, string,          image_path);
      KWD_ARG(Data, const char*,     include_paths_c_str);
      KWD_ARG(Data, const char**,    include_paths_array);
      KWD_ARG(Data, vector<string>&, include_paths);
    };

    Context(Data initializers)
    : mem(Memory_Manager<AST_Node*>()),
      sources(vector<const char*>()),
      include_paths(initializers.include_paths()),
      queue(vector<pair<string, const char*> >()),
      style_sheets(map<string, Block*>()),
      image_path(initializers.image_path())
    {
      collect_include_paths(initializers.include_paths_c_str());
      collect_include_paths(initializers.include_paths_array());
      add_file             (initializers.entry_point());
    }

    ~Context();
  };

}