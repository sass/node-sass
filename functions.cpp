#include "functions.hpp"
#include <iostream>
#
using std::cerr; using std::endl;

namespace Sass {
  namespace Functions {

    // TO DO: functions need to check the types of their arguments

    Function_Descriptor rgb_descriptor = 
    { "rgb", "$red", "$green", "$blue", 0 };
    Node rgb(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(Node::numeric_color, 0, 3);
      color << bindings[parameters[0]]
            << bindings[parameters[1]]
            << bindings[parameters[2]];
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
      return bindings[parameters[0]].clone() << bindings[parameters[1]];
    }
    
    
  }
}