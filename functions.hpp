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
    // RGB Functions ///////////////////////////////////////////////////////
    extern Function_Descriptor rgb_descriptor;
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings);

    extern Function_Descriptor rgba_4_descriptor;
    Node rgba_4(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor rgba_2_descriptor;
    Node rgba_2(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor red_descriptor;
    Node red(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor green_descriptor;
    Node green(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor blue_descriptor;
    Node blue(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor mix_2_descriptor;
    Node mix_2(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor mix_3_descriptor;
    Node mix_3(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    // HSL Functions ///////////////////////////////////////////////////////
    extern Function_Descriptor invert_descriptor;
    Node invert(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    // Opacity Functions ///////////////////////////////////////////////////
    extern Function_Descriptor alpha_descriptor;
    extern Function_Descriptor opacity_descriptor;
    Node alpha(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    // String Functions ////////////////////////////////////////////////////
    extern Function_Descriptor unquote_descriptor;
    Node unquote(const vector<Token>& parameters, map<Token, Node>& bindings);
    
    extern Function_Descriptor quote_descriptor;
    Node quote(const vector<Token>& parameters, map<Token, Node>& bindings);
  }
  
}
