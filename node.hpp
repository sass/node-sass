#include <vector>
#include <sstream>
#include "token.hpp"

namespace Sass {
  using std::vector;
  using std::stringstream;
  
  struct Node {
    enum Node_Type {
      nil,
      comment,
      ruleset,
      clauses,
      selector_group,
      selector,
      simple_selector_sequence,
      simple_selector,
      rule,
      property,
      values,
      value
    };
    
    static unsigned int fresh;
    static unsigned int copied;
    
    Node_Type type;
    Token token;
    vector<Node>* children;
    vector<Node>* opt_children;
    
    Node();
    Node(Node_Type _type);
    Node(Node_Type _type, Token& _token);
    Node(const Node& n);
    
    void push_child(const Node& node);
    void push_opt_child(const Node& node);
    void dump(unsigned int depth = 0);
    void emit_nested_css(stringstream& buf, const string& prefix, size_t depth);
    void emit_expanded_css(stringstream& buf, const string& prefix);
  };
  
  // struct Node {
  //   enum Type {
  //     nil,
  //     comment,
  //     ruleset,
  //     selector_group,
  //     selector,
  //     simple_selector_sequence,
  //     type_selector,
  //     class_selector,
  //     id_selector,
  //     attribute_selector,
  //     clauses,
  //     rule,
  //     property,
  //     values,
  //     value
  //   };
  //   
  //   bool has_rules;
  //   bool has_rulesets;
  //   Type type;
  //   Token token;
  //   vector<Node>* children;
  // 
  //   Node(const Node& n)
  //   : has_rules(n.has_rules), has_rulesets(n.has_rulesets),
  //     type(n.type), token(n.token), children(n.children)
  //   { n.children = 0; } // No joint custody.
  //   Node(Type type_)
  //   : has_rules(false), has_rulesets(false),
  //     type(type_), token(Token()), children(0)
  //   { }
  //   Node(Type type_, size_t size);
  //   Node(Type type_, Token& token_);
  //   
  //   Node& operator=(const Node& node);
  //   Node& operator+=(const Node& node);
  //   Node& operator<<(const Node& node);
  // };
}