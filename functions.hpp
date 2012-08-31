#define SASS_FUNCTIONS

#include <cstring>
#include <map>

#ifndef SASS_NODE
#include "node.hpp"
#endif

namespace Sass {
  using std::map;
  
  typedef Node (*Primitive)(const Node, map<Token, Node>&, Node_Factory& new_Node);
  typedef const char* str;
  typedef str Function_Descriptor[];

  struct Environment;
  
  struct Function {
    
    string name;
    // vector<Token> parameters;
    Node parameters;
    Node definition;
    Primitive primitive;
    bool overloaded;
    
    Function()
    { /* TO DO: set up the generic callback here */ }

    // for user-defined functions
    Function(Node def)
    : name(def[0].to_string()),
      parameters(def[1]),
      definition(def),
      primitive(0),
      overloaded(false)
    { }

    // Stub for overloaded primitives
    Function(string name, bool overloaded = true)
    : name(name),
      parameters(Node()),
      definition(Node()),
      primitive(0),
      overloaded(overloaded)
    { }

    Function(const char* signature, Primitive ip, Node_Factory& new_Node);
    
    Function(Function_Descriptor d, Primitive ip, Node_Factory& new_Node)
    : name(d[0]),
      parameters(new_Node(Node::parameters, "[PRIMITIVE FUNCTIONS]", 0, 0)),
      definition(Node()),
      primitive(ip),
      overloaded(false)
    {
      size_t len = 0;
      while (d[len+1]) ++len;
      
      for (size_t i = 0; i < len; ++i) {
        const char* p = d[i+1];
        parameters.push_back(new_Node(Node::variable, "[PRIMITIVE FUNCTIONS]", 0, Token::make(p, p + std::strlen(p))));
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
    Node rgb(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor rgba_4_descriptor;
    Node rgba_4(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor rgba_2_descriptor;
    Node rgba_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor red_descriptor;
    Node red(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor green_descriptor;
    Node green(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor blue_descriptor;
    Node blue(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor mix_2_descriptor;
    Node mix_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor mix_3_descriptor;
    Node mix_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    extern Function_Descriptor hsla_descriptor;
    Node hsla(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor hsl_descriptor;
    Node hsl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor adjust_hue_descriptor;
    Node adjust_hue(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor invert_descriptor;
    Node invert(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Opacity Functions ///////////////////////////////////////////////////

    extern Function_Descriptor alpha_descriptor;
    extern Function_Descriptor opacity_descriptor;
    Node alpha(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor opacify_descriptor;
    extern Function_Descriptor fade_in_descriptor;
    Node opacify(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor transparentize_descriptor;
    extern Function_Descriptor fade_out_descriptor;
    Node transparentize(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // String Functions ////////////////////////////////////////////////////

    extern Function_Descriptor unquote_descriptor;
    Node unquote(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor quote_descriptor;
    Node quote(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Number Functions ////////////////////////////////////////////////////

    extern Function_Descriptor percentage_descriptor;
    Node percentage(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor round_descriptor;
    Node round(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor ceil_descriptor;
    Node ceil(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor floor_descriptor;
    Node floor(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor abs_descriptor;    
    Node abs(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // List Functions //////////////////////////////////////////////////////
    
    extern Function_Descriptor length_descriptor;
    Node length(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor nth_descriptor;
    Node nth(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor join_2_descriptor;    
    Node join_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor join_3_descriptor;    
    Node join_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor append_2_descriptor;
    Node append_2(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor append_3_descriptor;
    Node append_3(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

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
    extern Function_Descriptor compact_11_descriptor;
    extern Function_Descriptor compact_12_descriptor;
    Node compact(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Introspection Functions /////////////////////////////////////////////
    
    extern Function_Descriptor type_of_descriptor;
    Node type_of(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor unit_descriptor;
    Node unit(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor unitless_descriptor;    
    Node unitless(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    extern Function_Descriptor comparable_descriptor;    
    Node comparable(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);
    
    // Boolean Functions ///////////////////////////////////////////////////
    
    extern Function_Descriptor not_descriptor;
    Node not_impl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

    extern Function_Descriptor if_descriptor;
    Node if_impl(const Node parameters, map<Token, Node>& bindings, Node_Factory& new_Node);

  }
  
}
