#include <cstring>
#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

namespace Sass {
  using std::map;
  
  typedef Node (*Implementation)(const vector<Token>&, map<Token, Node>&);
  typedef const char* str;
  typedef str Function_Descriptor[];
  
  struct Function {
    
    string name;
    vector<Token> parameters;
    Implementation implementation;
    
    Function()
    { /* TO DO: set up the generic callback here */ }
    
    Function(Function_Descriptor d, Implementation ip)
    : name(d[0]),
      parameters(vector<Token>()),
      implementation(ip)
    {
      size_t len = 0;
      while (d[len+1]) ++len;
      
      parameters.reserve(len);
      for (int i = 0; i < len; ++i) {
        const char* p = d[i+1];
        Token name(Token::make(p, p + std::strlen(p)));
        parameters.push_back(name);
      }
    }
    
    Node operator()(map<Token, Node>& bindings) const
    { return implementation(parameters, bindings); }

  };
  
  namespace Functions {
    extern Function_Descriptor rgb_descriptor;
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings);

    extern Function_Descriptor rgba_descriptor;
    Node rgba(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor curse_descriptor;
    Node curse(const vector<Token>& parameters, map<Token, Node>& bindings);
  }
  
}
