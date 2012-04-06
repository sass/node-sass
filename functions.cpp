#include "functions.hpp"
#include <iostream>
#include <cmath>
using std::cerr; using std::endl;

namespace Sass {
  namespace Functions {

    // TO DO: functions need to check the types of their arguments

    // RGB Functions ///////////////////////////////////////////////////////

    Function_Descriptor rgb_descriptor = 
    { "rgb", "$red", "$green", "$blue", 0 };
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(Node::numeric_color, 0, 4);
      color << bindings[parameters[0]]
            << bindings[parameters[1]]
            << bindings[parameters[2]]
            << Node(0, 1.0);
      return color;
    }

    Function_Descriptor rgba_4_descriptor = 
    { "rgba", "$red", "$green", "$blue", "$alpha", 0 };
    Node rgba_4(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(Node::numeric_color, 0, 4);
      color << bindings[parameters[0]]
            << bindings[parameters[1]]
            << bindings[parameters[2]]
            << bindings[parameters[3]];
      return color;
    }
    
    Function_Descriptor rgba_2_descriptor = 
    { "rgba", "$color", "$alpha", 0 };
    Node rgba_2(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(bindings[parameters[0]].clone());
      color[3] = bindings[parameters[1]];
      return color;
    }
    
    Function_Descriptor red_descriptor =
    { "red", "$color", 0 };
    Node red(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return bindings[parameters[0]][0];
    }
    
    Function_Descriptor green_descriptor =
    { "green", "$color", 0 };
    Node green(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return bindings[parameters[0]][1];
    }
    
    Function_Descriptor blue_descriptor =
    { "blue", "$color", 0 };
    Node blue(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return bindings[parameters[0]][2];
    }
    
    Node mix_impl(Node color1, Node color2, double weight = 50) {
      double p = weight/100;
      double w = 2*p - 1;
      double a = color1[3].content.numeric_value - color2[3].content.numeric_value;
      
      double w1 = (((w * a == -1) ? w : (w + a)/(1 + w*a)) + 1)/2.0;
      double w2 = 1 - w1;
      
      Node mixed(Node::numeric_color, color1.line_number, 4);
      for (int i = 0; i < 3; ++i) {
        mixed << Node(mixed.line_number, w1*color1[i].content.numeric_value +
                                         w2*color2[i].content.numeric_value);
      }
      double alpha = color1[3].content.numeric_value*p + color2[3].content.numeric_value*(1-p);
      mixed << Node(mixed.line_number, alpha);
      return mixed;
    }
    
    Function_Descriptor mix_2_descriptor =
    { "mix", "$color1", "$color2", 0 };
    Node mix_2(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return mix_impl(bindings[parameters[0]], bindings[parameters[1]]);
    }
    
    Function_Descriptor mix_3_descriptor =
    { "mix", "$color1", "$color2", "$weight", 0 };
    Node mix_3(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return mix_impl(bindings[parameters[0]],
                      bindings[parameters[1]],
                      bindings[parameters[2]].content.numeric_value);
    }
    
    // HSL Functions ///////////////////////////////////////////////////////
    
    Function_Descriptor invert_descriptor =
    { "invert", "$color", 0 };
    Node invert(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node orig(bindings[parameters[0]]);
      return Node(orig.line_number,
                  255 - orig[0].content.numeric_value,
                  255 - orig[1].content.numeric_value,
                  255 - orig[2].content.numeric_value,
                  orig[3].content.numeric_value);
    }
    
    // Opacity Functions ///////////////////////////////////////////////////
    
    Function_Descriptor alpha_descriptor =
    { "alpha", "$color", 0 };
    Function_Descriptor opacity_descriptor =
    { "opacity", "$color", 0 };
    Node alpha(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return bindings[parameters[0]][3];
    }
    
    Function_Descriptor opacify_descriptor =
    { "opacify", "$color", "$amount", 0 };
    Function_Descriptor fade_in_descriptor =
    { "fade_in", "$color", "$amount", 0 };
    Node opacify(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy[3].content.numeric_value += bindings[parameters[1]].content.numeric_value;
      if (cpy[3].content.numeric_value >= 1) cpy[3].content.numeric_value = 1;
      return cpy;
    }
    
    Function_Descriptor transparentize_descriptor =
    { "transparentize", "$color", "$amount", 0 };
    Function_Descriptor fade_out_descriptor =
    { "fade_out", "$color", "$amount", 0 };
    Node transparentize(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy[3].content.numeric_value -= bindings[parameters[1]].content.numeric_value;
      if (cpy[3].content.numeric_value <= 0) cpy[3].content.numeric_value = 0;
      return cpy;
    }
      
    // String Functions ////////////////////////////////////////////////////
    
    Function_Descriptor unquote_descriptor =
    { "unquote", "$string", 0 };
    Node unquote(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      cpy.unquoted = true;
      return cpy;
    }
    
    Function_Descriptor quote_descriptor =
    { "quote", "$string", 0 };
    Node quote(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      // check the types -- will probably be an identifier
      cpy.type = Node::string_constant;
      cpy.unquoted = false;
      return cpy;
    }
    
    // Number Functions ////////////////////////////////////////////////////
    
    Function_Descriptor percentage_descriptor =
    { "percentage", "$value", 0 };
    Node percentage(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      // TO DO: make sure it's not already a percentage
      cpy.content.numeric_value = cpy.content.numeric_value * 100;
      cpy.type = Node::numeric_percentage;
      return cpy;
    }

    Function_Descriptor round_descriptor =
    { "round", "$value", 0 };
    Node round(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::floor(cpy.content.dimension.numeric_value + 0.5);
      }
      else {
        cpy.content.numeric_value = std::floor(cpy.content.numeric_value + 0.5);
      }
      return cpy;
    }

    Function_Descriptor ceil_descriptor =
    { "ceil", "$value", 0 };
    Node ceil(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::ceil(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::ceil(cpy.content.numeric_value);
      }
      return cpy;
    }

    Function_Descriptor floor_descriptor =
    { "floor", "$value", 0 };
    Node floor(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::floor(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::floor(cpy.content.numeric_value);
      }
      return cpy;
    }

    Function_Descriptor abs_descriptor =
    { "abs", "$value", 0 };
    Node abs(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node cpy(bindings[parameters[0]].clone());
      if (cpy.type == Node::numeric_dimension) {
        cpy.content.dimension.numeric_value = std::fabs(cpy.content.dimension.numeric_value);
      }
      else {
        cpy.content.numeric_value = std::fabs(cpy.content.numeric_value);
      }
      return cpy;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  }
}