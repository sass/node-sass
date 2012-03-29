#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#ifndef SASS_CONTEXT_INCLUDED
#include "context.hpp"
#endif

namespace Sass {
  using std::map;
  
  Node eval(const Node& expr, Environment& env);
  Node accumulate(const Node::Type op, Node& acc, Node& rhs);
  double operate(const Node::Type op, double lhs, double rhs);
  
  Node apply(const Node& mixin, const Node& args, Environment& env);
}