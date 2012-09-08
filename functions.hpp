#define SASS_FUNCTIONS

#include <cstring>
#include <map>

#ifndef SASS_NODE
#include "node.hpp"
#endif

namespace Sass {
  struct Environment;
  struct Context;

  using std::map;
  
  typedef Node (*Primitive)(const Node, Environment&, Node_Factory&);
  typedef const char Signature[];

  struct Function {
    
    string name;
    Node parameters;
    Node parameter_names;
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

    Function(char* signature, Primitive ip, Context& ctx);

    Node operator()(Environment& bindings, Node_Factory& new_Node) const
    {
      if (primitive) return primitive(parameters, bindings, new_Node);
      else           return Node();
    }

  };
  
  namespace Functions {

    // RGB Functions ///////////////////////////////////////////////////////

    extern Signature rgb_sig;
    Node rgb(const Node, Environment&, Node_Factory&);

    extern Signature rgba_4_sig;
    Node rgba_4(const Node, Environment&, Node_Factory&);
    
    extern Signature rgba_2_sig;
    Node rgba_2(const Node, Environment&, Node_Factory&);
    
    extern Signature red_sig;
    Node red(const Node, Environment&, Node_Factory&);
    
    extern Signature green_sig;
    Node green(const Node, Environment&, Node_Factory&);
    
    extern Signature blue_sig;
    Node blue(const Node, Environment&, Node_Factory&);
    
    extern Signature mix_sig;
    Node mix(const Node, Environment&, Node_Factory&);
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    extern Signature hsl_sig;
    Node hsl(const Node, Environment&, Node_Factory&);

    extern Signature hsla_sig;
    Node hsla(const Node, Environment&, Node_Factory&);
    
    extern Signature hue_sig;
    Node hue(const Node, Environment&, Node_Factory&);

    extern Signature saturation_sig;
    Node saturation(const Node, Environment&, Node_Factory&);

    extern Signature lightness_sig;
    Node lightness(const Node, Environment&, Node_Factory&);

    extern Signature adjust_hue_sig;
    Node adjust_hue(const Node, Environment&, Node_Factory&);

    extern Signature lighten_sig;
    Node lighten(const Node, Environment&, Node_Factory&);

    extern Signature darken_sig;
    Node darken(const Node, Environment&, Node_Factory&);

    extern Signature saturate_sig;
    Node saturate(const Node, Environment&, Node_Factory&);

    extern Signature desaturate_sig;
    Node desaturate(const Node, Environment&, Node_Factory&);

    extern Signature grayscale_sig;
    Node grayscale(const Node, Environment&, Node_Factory&);    

    extern Signature complement_sig;
    Node complement(const Node, Environment&, Node_Factory&);    

    extern Signature invert_sig;
    Node invert(const Node, Environment&, Node_Factory&);
    
    // Opacity Functions ///////////////////////////////////////////////////

    extern Signature alpha_sig;
    Node alpha(const Node, Environment&, Node_Factory&);

    extern Signature opacity_sig;
    Node opacity(const Node, Environment&, Node_Factory&);    
    
    extern Signature opacify_sig;
    Node opacify(const Node, Environment&, Node_Factory&);

    extern Signature fade_in_sig;
    Node fade_in(const Node, Environment&, Node_Factory&);
    
    extern Signature transparentize_sig;
    Node transparentize(const Node, Environment&, Node_Factory&);

    extern Signature fade_out_sig;
    Node fade_out(const Node, Environment&, Node_Factory&);

    // Other Color Functions ///////////////////////////////////////////////
    
    extern Signature adjust_color_sig;
    Node adjust_color(const Node, Environment&, Node_Factory&);

    extern Signature change_color_sig;
    Node change_color(const Node, Environment&, Node_Factory&);

    // String Functions ////////////////////////////////////////////////////

    extern Signature unquote_sig;
    Node unquote(const Node, Environment&, Node_Factory&);
    
    extern Signature quote_sig;
    Node quote(const Node, Environment&, Node_Factory&);
    
    // Number Functions ////////////////////////////////////////////////////

    extern Signature percentage_sig;
    Node percentage(const Node, Environment&, Node_Factory&);

    extern Signature round_sig;
    Node round(const Node, Environment&, Node_Factory&);

    extern Signature ceil_sig;
    Node ceil(const Node, Environment&, Node_Factory&);

    extern Signature floor_sig;
    Node floor(const Node, Environment&, Node_Factory&);

    extern Signature abs_sig;    
    Node abs(const Node, Environment&, Node_Factory&);
    
    // List Functions //////////////////////////////////////////////////////
    
    extern Signature length_sig;
    Node length(const Node, Environment&, Node_Factory&);

    extern Signature nth_sig;
    Node nth(const Node, Environment&, Node_Factory&);

    extern Signature join_sig;    
    Node join(const Node, Environment&, Node_Factory&);

    extern Signature append_sig;
    Node append(const Node, Environment&, Node_Factory&);

    extern Signature compact_sig;
    Node compact(const Node, Environment&, Node_Factory&);
    
    // Introspection Functions /////////////////////////////////////////////
    
    extern Signature type_of_sig;
    Node type_of(const Node, Environment&, Node_Factory&);

    extern Signature unit_sig;
    Node unit(const Node, Environment&, Node_Factory&);
    
    extern Signature unitless_sig;    
    Node unitless(const Node, Environment&, Node_Factory&);
    
    extern Signature comparable_sig;    
    Node comparable(const Node, Environment&, Node_Factory&);
    
    // Boolean Functions ///////////////////////////////////////////////////
    
    extern Signature not_sig;
    Node not_impl(const Node, Environment&, Node_Factory&);

    extern Signature if_sig;
    Node if_impl(const Node, Environment&, Node_Factory&);

  }
  
}
