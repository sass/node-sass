#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

namespace Sass {
  using std::map;
  
  Node eval(const Node& expr, map<Token, Node>& g_env);
  Node accumulate(const Node::Type op, Node& acc, Node& rhs);
  double operate(const Node::Type op, double lhs, double rhs);
}