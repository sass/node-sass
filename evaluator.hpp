#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

namespace Sass {
  Node eval(const Node& expr);
  Node accumulate(const Node::Type op, Node& acc, Node& rhs);
  double operate(const Node::Type op, double lhs, double rhs);
}