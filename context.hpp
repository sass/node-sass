namespace Sass {
  using std::map;
  
  struct Context {
    map<Token, Node> environment;
    // Environment environment;
    // map<Token, Node> mixins;
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
  
  struct Environment {
    vector< map<Token, Node> > stack;
    
    Environment()
    : stack(vector< map<Token, Node> >())
    {
      stack.reserve(2);
      stack.push_back(map<Token, Node>());
    }
    
    void extend()
    {
      stack.push_back(map<Token, Node>());
    }
    
  };
}