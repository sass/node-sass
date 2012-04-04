namespace Sass {
  
  struct Environment;
  
  struct Function {
    
    typedef Node (*Primitive)(const Node&, const Environment&);
    
    string name;
    Node parameters;
    vector<Token> param_names;
    Primitive primitive;
    
    Function()
    { }
    
    Function(string name, Node parameters, Primitive primitive)
    : name(name), parameters(parameters), primitive(primitive)
    { }
    
    Node operator()(const Environment& bindings) const
    { return primitive(parameters, bindings); }

  };
  
}
