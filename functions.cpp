#include "functions.hpp"
#include "node.hpp"

namespace Sass {
  namespace Functions {
  
  extern const char* rgb_metadata[] = {
    "rgb",
    "$red",
    "$green",
    "$blue",
    0
  };
  Node rgb(const vector<Token>& param_names, const Environment& bindings) {
    Node color(Node::numeric_color, 0, 3);
    color << bindings[param_names[0]]
          << bindings[param_names[1]]
          << bindings[param_names[2]];
    return color;
  }
  
  extern const char* rgba_metadata[] = {
    "rgba",
    "$red",
    "$green",
    "$blue",
    "$alpha",
    0
  };
  Node rgba(const vector<Token>& param_names, const Environment& bindings) {
    Node color(Node::numeric_color, 0, 3);
    color << bindings[param_names[0]]
          << bindings[param_names[1]]
          << bindings[param_names[2]]
          << bindings[param_names[3]];
    return color;
  }
  
}