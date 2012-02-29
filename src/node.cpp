#include "node.hpp"

namespace Sass {
  Node::Node() { }
  Node::Node(Node_Type _type) {
    type = _type;
  }
  Node::Node(Node_Type _type, Token& _token) {
    type = _type;
    token = _token;
  }
  void Node::push_child(const Node& node) {
    children.push_back(node);
  }
  void Node::push_opt_child(const Node& node) {
    opt_children.push_back(node);
  }
}