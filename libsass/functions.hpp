#include <cstring>
#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

namespace Sass {
  using std::map;
  
  typedef Node (*Primitive)(const vector<Token>&, map<Token, Node>&, Node_Factory& new_Node);
  typedef const char* str;
  typedef str Function_Descriptor[];
  
  struct Function {
    
    string name;
    vector<Token> parameters;
    Node definition;
    Primitive primitive;
    
    Function()
    { /* TO DO: set up the generic callback here */ }

    Function(Node def)
    : name(def[0].to_string()),
      parameters(vector<Token>()),
      definition(def),
      primitive(0)
    { }
    
    Function(Function_Descriptor d, Primitive ip)
    : name(d[0]),
      parameters(vector<Token>()),
      definition(Node()),
      primitive(ip)
    {
      size_t len = 0;
      while (d[len+1]) ++len;
      
      parameters.reserve(len);
      for (size_t i = 0; i < len; ++i) {
        const char* p = d[i+1];
        Token name(Token::make(p, p + std::strlen(p)));
        parameters.push_back(name);
      }
    }
    
    Node operator()(map<Token, Node>& bindings, Node_Factory& new_Node) const
    {
      if (primitive) return primitive(parameters, bindings, new_Node);
      else           return Node();
    }

  };
  
  namespace Functions {

    // RGB Functions ///////////////////////////////////////////////////////

    extern Function_Descriptor rgb_descriptor;
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor rgba_4_descriptor;
    Node rgba_4(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor rgba_2_descriptor;
    Node rgba_2(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor red_descriptor;
    Node red(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor green_descriptor;
    Node green(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor blue_descriptor;
    Node blue(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor mix_2_descriptor;
    Node mix_2(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor mix_3_descriptor;
    Node mix_3(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    extern Function_Descriptor hsla_descriptor;
    Node hsla(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor hsl_descriptor;
    Node hsl(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor invert_descriptor;
    Node invert(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Opacity Functions ///////////////////////////////////////////////////

    extern Function_Descriptor alpha_descriptor;
    extern Function_Descriptor opacity_descriptor;
    Node alpha(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor opacify_descriptor;
    extern Function_Descriptor fade_in_descriptor;
    Node opacify(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor transparentize_descriptor;
    extern Function_Descriptor fade_out_descriptor;
    Node transparentize(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // String Functions ////////////////////////////////////////////////////

    extern Function_Descriptor unquote_descriptor;
    Node unquote(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor quote_descriptor;
    Node quote(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Number Functions ////////////////////////////////////////////////////

    extern Function_Descriptor percentage_descriptor;
    Node percentage(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor round_descriptor;
    Node round(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor ceil_descriptor;
    Node ceil(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor floor_descriptor;
    Node floor(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor abs_descriptor;    
    Node abs(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // List Functions //////////////////////////////////////////////////////
    
    extern Function_Descriptor length_descriptor;
    Node length(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor nth_descriptor;
    Node nth(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor join_2_descriptor;    
    Node join_2(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor join_3_descriptor;    
    Node join_3(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor append_2_descriptor;
    Node append_2(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor append_3_descriptor;
    Node append_3(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor compact_1_descriptor;
    extern Function_Descriptor compact_2_descriptor;
    extern Function_Descriptor compact_3_descriptor;
    extern Function_Descriptor compact_4_descriptor;
    extern Function_Descriptor compact_5_descriptor;
    extern Function_Descriptor compact_6_descriptor;
    extern Function_Descriptor compact_7_descriptor;
    extern Function_Descriptor compact_8_descriptor;
    extern Function_Descriptor compact_9_descriptor;
    extern Function_Descriptor compact_10_descriptor;
    Node compact(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Introspection Functions /////////////////////////////////////////////
    
    extern Function_Descriptor type_of_descriptor;
    Node type_of(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor unit_descriptor;
    Node unit(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor unitless_descriptor;    
    Node unitless(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor comparable_descriptor;    
    Node comparable(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Boolean Functions ///////////////////////////////////////////////////
    
    extern Function_Descriptor not_descriptor;
    Node not_impl(const vector<Token>& parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

  }
  
}
