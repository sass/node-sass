#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#ifndef SASS_CONTEXT_INCLUDED
#include "context.hpp"
#endif

namespace Sass {
  using std::map;
  
  Node eval(Node& expr, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node);
  Node accumulate(Node::Type op, Node& acc, Node& rhs, Node_Factory& new_Node);
  double operate(Node::Type op, double lhs, double rhs);
  
  Node apply_mixin(Node& mixin, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node);
  Node apply_function(const Function& f, const Node& args, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node);
}