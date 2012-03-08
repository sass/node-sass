#include <vector>
#include <sstream>
#include "token.hpp"

namespace Sass {
  using std::vector;
  using std::stringstream;
  
  // struct Node {
  //   enum Node_Type {
  //     nil,
  //     comment,
  //     ruleset,
  //     clauses,
  //     selector_group,
  //     selector,
  //     simple_selector_sequence,
  //     simple_selector,
  //     rule,
  //     property,
  //     values,
  //     value
  //   };
  //   
  //   static unsigned int fresh;
  //   static unsigned int copied;
  //   
  //   Node_Type type;
  //   Token token;
  //   vector<Node>* children;
  //   vector<Node>* opt_children;
  //   
  //   Node();
  //   Node(Node_Type _type);
  //   Node(Node_Type _type, Token& _token);
  //   Node(const Node& n);
  //   
  //   void push_child(const Node& node);
  //   void push_opt_child(const Node& node);
  //   void dump(unsigned int depth = 0);
  //   void emit_nested_css(stringstream& buf, const string& prefix, size_t depth);
  //   void emit_expanded_css(stringstream& buf, const string& prefix);
  // };
  
  struct Node {
    enum Type {
      nil,
      comment,
      ruleset,
      propset,
      selector_group,
      selector,
      simple_selector_sequence,
      type_selector,
      class_selector,
      id_selector,
      attribute_selector,
      block,
      rule,
      property,
      values,
      value
    };
    
    static size_t fresh;
    static size_t copied;
    
    size_t line_number;
    mutable vector<Node>* children;
    Token token;
    Type type;
    bool has_comments;
    bool has_rules;
    bool has_rulesets;
    bool has_propsets;
    
    Node() : type(nil), children(0) { ++fresh; }
  
    Node(const Node& n)
    : line_number(n.line_number),
      children(n.children),
      token(n.token),
      type(n.type),
      has_rules(n.has_rules),
      has_rulesets(n.has_rulesets),
      has_propsets(n.has_propsets)
    { /*n.release();*/ ++copied; } // No joint custody.
  
    Node(size_t line_number, Type type, size_t length = 0)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      type(type),
      has_rules(false),
      has_rulesets(false),
      has_propsets(false)
    { children->reserve(length); ++fresh; }
    
    Node(size_t line_number, Type type, const Node& n)
    : line_number(line_number),
      children(new vector<Node>(1, n)),
      token(Token()),
      type(type),
      has_rules(false),
      has_rulesets(false),
      has_propsets(false)
    { ++fresh; }
    
    Node(size_t line_number, Type type, const Node& n, const Node& m)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      type(type),
      has_rules(false),
      has_rulesets(false),
      has_propsets(false)
    {
      children->reserve(2);
      children->push_back(n);
      children->push_back(m);
      ++fresh;
    }
  
    Node(size_t line_number, Type type, Token& token)
    : line_number(line_number),
      children(0),
      token(token),
      type(type),
      has_rules(false),
      has_rulesets(false),
      has_propsets(false)
    { ++fresh; }
    
    //~Node() { delete children; }
    
    Node& operator=(const Node& n)
    {
      line_number = n.line_number;
      children = n.children;
      // n.release();
      token = n.token;
      type = n.type;
      has_rules = n.has_rules;
      has_rulesets = n.has_rulesets;
      has_propsets = n.has_propsets;
      ++copied;
      return *this;
    }
      
    // Node& operator+=(const Node& node);
    Node& operator<<(const Node& n)
    {
      children->push_back(n);
      return *this;
    }
    
    void release() const { children = 0; }
    
    void emit_nested_css(stringstream& buf, const string& prefix, size_t depth);
    void emit_expanded_css(stringstream& buf, const string& prefix);
  };
}