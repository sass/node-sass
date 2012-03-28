#define SASS_CONTEXT_INCLUDED

namespace Sass {
  using std::map;
  
  struct Environment {
    map<Token, Node> frame;
    Environment* parent;
    
    Environment() : frame(map<Token, Node>()), parent(0)
    { }
    Environment(Environment* env) : frame(map<Token, Node>()), parent(env)
    { }
    
    bool query(const Token& key) const
    {
      if (frame.count(key)) return true;
      else if (parent)      return parent->query(key);
      else                  return false;
    }
    
    Node& operator[](const Token& key)
    {
      if (frame.count(key)) return frame[key];
      else if (parent)      return (*parent)[key];
      else                  return frame[key];
    }
  };

  struct Context {
    map<Token, Node> environment;
    Environment global_env;
    vector<Node> pending;
    vector<char*> source_refs;
    size_t ref_count;

    Context()
    : environment(map<Token, Node>()),
      // mixins(map<Token, Node>()),
      pending(vector<Node>()),
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
