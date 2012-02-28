#include <vector>
#include "token.hpp"

namespace Sass {
  using std::vector;
  struct Node {
    enum Node_Type {
      null,
      comment,
      rule_set,
      declaration,
      selector_group,
      selector,
      simple_selector_sequence,
      simple_selector,
      property,
      value,
      lookahead_sequence,
      lookahead_token
    };
    
    Node_Type type;
    Token token;
    vector<Node> children;
    vector<Node> opt_children;
    
    Node();
    Node(Node_Type _type, Token& _token);
    void push_child(Node& node);
    void push_opt_child(Node& node);
  };
}