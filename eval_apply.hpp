#include <map>

#ifndef SASS_NODE_INCLUDED
#include "node.hpp"
#endif

#ifndef SASS_CONTEXT_INCLUDED
#include "context.hpp"
#endif

namespace Sass {
  using std::map;
  
  Node eval(Node expr, Node prefix, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node, Context& ctx);
  Node function_eval(string name, Node stm, Environment& bindings, Node_Factory& new_Node, Context& ctx, bool toplevel = false);
  Node accumulate(Node::Type op, Node acc, Node rhs, Node_Factory& new_Node);
  double operate(Node::Type op, double lhs, double rhs);
  
  Node apply_mixin(Node mixin, const Node args, Node prefix, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node, Context& ctx, bool dynamic_scope = false);
  Node apply_function(const Function& f, const Node args, Node prefix, Environment& env, map<pair<string, size_t>, Function>& f_env, Node_Factory& new_Node, Context& ctx);
  Node expand_selector(Node sel, Node pre, Node_Factory& new_Node);
  Node expand_backref(Node sel, Node pre);
  void extend_selectors(vector<pair<Node, Node> >&, Node_Factory&);
  Node generate_extension(Node extendee, Node extender, Node_Factory& new_Node);

  Node selector_prefix(Node sel, Node_Factory& new_Node);
  Node selector_base(Node sel);

  Node selector_butfirst(Node sel, Node_Factory& new_Node);
  Node selector_butlast(Node sel, Node_Factory& new_Node);

}