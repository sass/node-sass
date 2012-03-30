#define SASS_NODE_INCLUDED

#include <vector>
#include <sstream>
#include "token.hpp"

namespace Sass {
  using std::string;
  using std::vector;
  using std::stringstream;

  struct Node {
    enum Type {
      root,
      ruleset,
      propset,

      selector_group,
      selector,
      selector_combinator,
      simple_selector_sequence,
      backref,
      simple_selector,
      type_selector,
      class_selector,
      id_selector,
      pseudo,
      pseudo_negation,
      functional_pseudo,
      attribute_selector,

      block,
      rule,
      property,

      nil,
      comma_list,
      space_list,

      expression,
      add,
      sub,

      term,
      mul,
      div,

      factor,
      values,
      value,
      identifier,
      uri,
      textual_percentage,
      textual_dimension,
      textual_number,
      textual_hex,
      string_constant,
      numeric_percentage,
      numeric_dimension,
      number,
      hex_triple,
      
      mixin,
      parameters,
      expansion,
      arguments,
      
      variable,
      assignment,
      
      comment,
      none,
      flags
    };
    
    static size_t fresh;
    static size_t copied;
    static size_t allocations;
    
    size_t line_number;
    mutable vector<Node>* children;
    Token token;
    double numeric_value;
    Type type;
    bool has_rules_or_comments;
    bool has_rulesets;
    bool has_propsets;
    bool has_expansions;
    bool has_backref;
    bool from_variable;
    bool eval_me;
    
    Node() : type(none), children(0) { ++fresh; }
    
    Node(Node::Type type)
    : type(type),
      children(0),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false)
    { ++fresh; }
  
    Node(const Node& n)
    : line_number(n.line_number),
      children(n.children),
      token(n.token),
      numeric_value(n.numeric_value),
      type(n.type),
      has_rules_or_comments(n.has_rules_or_comments),
      has_rulesets(n.has_rulesets),
      has_propsets(n.has_propsets),
      has_expansions(n.has_expansions),
      has_backref(n.has_backref),
      from_variable(n.from_variable),
      eval_me(n.eval_me)
    { ++copied; }
  
    Node(size_t line_number, Type type, size_t length = 0)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      numeric_value(0),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    { children->reserve(length); ++fresh; ++allocations; }
    
    Node(size_t line_number, Type type, const Node& n)
    : line_number(line_number),
      children(new vector<Node>(1, n)),
      token(Token()),
      numeric_value(0),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    { ++fresh; ++allocations; }
    
    Node(size_t line_number, Type type, const Node& n, const Node& m)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      numeric_value(0),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    {
      children->reserve(2);
      children->push_back(n);
      children->push_back(m);
      ++fresh;
      ++allocations;
    }
  
    Node(size_t line_number, Type type, Token& token)
    : line_number(line_number),
      children(0),
      token(token),
      numeric_value(0),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    { ++fresh; }
    
    Node(size_t line_number, double d)
    : line_number(line_number),
      children(0),
      token(Token()),
      numeric_value(d),
      type(number),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    { ++fresh; }
    
    Node(size_t line_number, double d, Token& token)
    : line_number(line_number),
      children(0),
      token(token),
      numeric_value(d),
      type(numeric_dimension),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    { ++fresh; }
    
    Node(size_t line_number, double a, double b, double c)
    : line_number(line_number),
      children(new vector<Node>()),
      token(Token()),
      numeric_value(0),
      type(hex_triple),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false),
      has_expansions(false),
      has_backref(false),
      from_variable(false),
      eval_me(false)
    {
      children->reserve(3);
      children->push_back(Node(line_number, a));
      children->push_back(Node(line_number, b));
      children->push_back(Node(line_number, c));
      ++fresh;
      ++allocations;
    }
    
    //~Node() { delete children; }
    
    size_t size() const
    { return children->size(); }
    
    Node& operator[](const size_t i) const
    { return children->at(i); }
    
    Node& at(const size_t i) const
    { return children->at(i); }
    
    Node& operator=(const Node& n)
    {
      line_number = n.line_number;
      children = n.children;
      token = n.token;
      numeric_value = n.numeric_value;
      type = n.type;
      has_rules_or_comments = n.has_rules_or_comments;
      has_rulesets = n.has_rulesets;
      has_propsets = n.has_propsets;
      has_backref = n.has_backref;
      from_variable = n.from_variable;
      eval_me = n.eval_me;
      ++copied;
      return *this;
    }
      
    Node& operator<<(const Node& n)
    {
      children->push_back(n);
      return *this;
    }
    
    Node& operator+=(const Node& n)
    {
      for (int i = 0; i < n.children->size(); ++i) {
        children->push_back(n.children->at(i));
      }
      return *this;
    }
    
    string to_string(const string& prefix) const;
    
    void release() const { children = 0; }
    
    Node clone() const;
    
    void echo(stringstream& buf, size_t depth = 0);
    void emit_nested_css(stringstream& buf,
                         size_t depth,
                         const vector<string>& prefixes);
    void emit_nested_css(stringstream& buf, size_t depth);
    void emit_expanded_css(stringstream& buf, const string& prefix);
    
    void flatten();
  };
}