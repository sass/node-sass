namespace Sass {
  using std::map;
  
  struct Context {
    map<Token, Node> environment;
    Context() : environment(map<Token, Node>()) { }
  };
}