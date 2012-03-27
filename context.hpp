namespace Sass {
  using std::map;
  
  struct Context {
    map<Token, Node> environment;
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
}