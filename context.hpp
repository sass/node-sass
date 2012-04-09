#define SASS_CONTEXT_INCLUDED

#include <utility>
#include <map>
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
    map<pair<string, size_t>, Function> function_env;
    vector<char*> source_refs;
    size_t ref_count;
    
    Context();
    ~Context();
    
    void register_function(Function_Descriptor d, Implementation ip);
    void register_function(Function_Descriptor d, Implementation ip, size_t arity);
    void register_functions();
  };

}
