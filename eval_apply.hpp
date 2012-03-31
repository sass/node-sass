#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#ifndef SASS_CONTEXT_INCLUDED
#include "context.hpp"
#endif

namespace Sass {
  using std::map;
  
  Node eval(Node& expr, Environment& env);
  Node accumulate(Node::Type op, Node& acc, Node& rhs);
  double operate(Node::Type op, double lhs, double rhs);
  
  Node apply(Node& mixin, const Node& args, Environment& env);
}