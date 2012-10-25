#define SASS_CONTEXT

//#ifndef SASS_ENVIRONMENT
#include "environment.hpp"
//#endif

#include <utility>

#ifndef SASS_NODE_FACTORY
#include "node_factory.hpp"
#endif

#ifndef SASS_FUNCTIONS
#include "functions.hpp"
#endif

namespace Sass {
  using std::pair;
  using std::map;
  
  struct Context {
    Environment global_env;
    map<string, Function> function_env;
    multimap<Node, Node> extensions;
    vector<pair<Node, Node> > pending_extensions;
    vector<char*> source_refs; // all the source c-strings
    vector<string> include_paths;
    map<string, Node> color_names_to_values;
    map<Node, string> color_values_to_names;
    Node_Factory new_Node;
    char* image_path;
    size_t ref_count;
    // string sass_path;
    // string css_path;
    bool has_extensions;

    void collect_include_paths(const char* paths_str);
    Context(const char* paths_str = 0, const char* img_path_str = 0);
    ~Context();

    void register_function(Signature sig, Primitive ip);
    void register_function(Signature sig, Primitive ip, size_t arity);
    void register_overload_stub(string name);
    void register_functions();
    void setup_color_map();
  };

}
