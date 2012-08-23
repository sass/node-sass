#define SASS_CONTEXT_INCLUDED


#include <utility>
#include <map>
#include "node_factory.hpp"
#include "functions.hpp"

namespace Sass {
  using std::pair;
  using std::map;
  
  struct Environment {
    map<Token, Node> current_frame;
    Environment* parent;
    Environment* global;
    
    Environment()
    : current_frame(map<Token, Node>()), parent(0), global(0)
    { }
    
    void link(Environment& env)
    {
      parent = &env;
      global = parent->global ? parent->global : parent;
    }
    
    bool query(const Token& key) const
    {
      if (current_frame.count(key)) return true;
      else if (parent)              return parent->query(key);
      else                          return false;
    }
    
    Node& operator[](const Token& key)
    {
      if (current_frame.count(key)) return current_frame[key];
      else if (parent)              return (*parent)[key];
      else                          return current_frame[key];
    }
  };

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
    
    void register_function(Function_Descriptor d, Primitive ip);
    void register_function(Function_Descriptor d, Primitive ip, size_t arity);
    void register_overload_stub(string name);
    void register_functions();
    void setup_color_map();
  };

}
