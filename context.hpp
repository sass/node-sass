#define SASS_CONTEXT_INCLUDED

namespace Sass {
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
    vector<Node> pending;
    vector<char*> source_refs;
    size_t ref_count;

    Context()
    : pending(vector<Node>()),
      source_refs(vector<char*>()),
      ref_count(0)
    { }
    
    ~Context()
    {
      for (int i = 0; i < source_refs.size(); ++i) {
        delete[] source_refs[i];
      }
    }
  };

}
