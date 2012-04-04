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

    Function_Descriptor rgba_descriptor = 
    { "rgba", "$red", "$green", "$blue", "$alpha", 0 };
    Node rgba(const vector<Token>& parameters, map<Token, Node>& bindings) {
      Node color(Node::numeric_color, 0, 4);
      color << bindings[parameters[0]]
            << bindings[parameters[1]]
            << bindings[parameters[2]]
            << bindings[parameters[3]];
      return color;
    }
    
    extern const char* the_curse = "Damn!";
    Function_Descriptor curse_descriptor = { "curse", 0 };
    Node curse(const vector<Token>& parameters, map<Token, Node>& bindings) {
      return Node(Node::identifier, 0, Token::make(the_curse, the_curse + std::strlen(the_curse)));
    }
  }
}