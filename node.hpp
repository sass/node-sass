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
  
  struct Node {
    enum Type {
      nil,
      comment,
      ruleset,
      selector_group,
      selector,
      simple_selector_sequence,
      type_selector,
      class_selector,
      id_selector,
      attribute_selector,
      clauses,
      rule,
      property,
      values,
      value
    };
    
  //   size_t line_number;
  //   mutable vector<Node>* children;
  //   Token token;
  //   Type type;
  //   bool has_rules;
  //   bool has_rulesets;
  //   bool has_nested_properties;
  // 
  //   Node(const Node& n)
  //   : line_number(n.line_number),
  //     children(n.children),
  //     token(n.token),
  //     type(n.type),
  //     has_rules(n.has_rules),
  //     has_rulesets(n.has_rulesets),
  //     has_nested_properties(n.has_nested_properties)
  //   { n.release(); } // No joint custody.
  // 
  //   Node(size_t line_number, Type type, size_t length = 0)
  //   : line_number(line_number),
  //     children(new vector<Node>),
  //     token(Token()),
  //     type(type),
  //     has_rules(false),
  //     has_rulesets(false),
  //     has_nested_properties(false)
  //   { children->reserve(length); }
  // 
  //   Node(size_t line_number, Type type, Token& token)
  //   : line_number(line_number),
  //     children(0),
  //     token(token),
  //     type(type),
  //     has_rules(false),
  //     has_rulesets(false),
  //     has_nested_properties(false)
  //   { }
  //   
  //   Node& operator=(const Node& node);
  //   Node& operator+=(const Node& node);
  //   Node& operator<<(const Node& node);
  //   void release() const { children = 0; }
  // };
}