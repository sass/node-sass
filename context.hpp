namespace Sass {
  using std::map;
  
  struct Context {
    map<Token, Node> environment;
    size_t ref_count;
    Context() : environment(map<Token, Node>()), ref_count(0) { }
  };
}