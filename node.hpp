#include <vector>
#include <sstream>
#include "token.hpp"

namespace Sass {
  using std::string;
  using std::vector;
  using std::stringstream;

  struct Node {
    enum Type {
      nil,
      comment,
      ruleset,
      propset,
      selector_group,
      selector,
      selector_combinator,
      simple_selector_sequence,
      simple_selector,
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
    bool has_rules_or_comments;
    bool has_rulesets;
    bool has_propsets;
    
    Node() : type(nil), children(0) { ++fresh; }
  
    Node(const Node& n)
    : line_number(n.line_number),
      children(n.children),
      token(n.token),
      type(n.type),
      has_rules_or_comments(n.has_rules_or_comments),
      has_rulesets(n.has_rulesets),
      has_propsets(n.has_propsets)
    { /*n.release();*/ ++copied; } // No joint custody.
  
    Node(size_t line_number, Type type, size_t length = 0)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false)
    { children->reserve(length); ++fresh; }
    
    Node(size_t line_number, Type type, const Node& n)
    : line_number(line_number),
      children(new vector<Node>(1, n)),
      token(Token()),
      type(type),
      has_rules_or_comments(false),
      has_rulesets(false),
      has_propsets(false)
    { ++fresh; }
    
    Node(size_t line_number, Type type, const Node& n, const Node& m)
    : line_number(line_number),
      children(new vector<Node>),
      token(Token()),
      type(type),
      has_rules_or_comments(false),
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
      has_rules_or_comments(false),
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
      has_rules_or_comments = n.has_rules_or_comments;
      has_rulesets = n.has_rulesets;
      has_propsets = n.has_propsets;
      ++copied;
      return *this;
    }
      
    Node& operator<<(const Node& n)
    {
      children->push_back(n);
      return *this;
    }
    
    operator string() {
      if (type == selector) {
        string result;
        if (children->at(0).type == selector_combinator) {
          result += string(children->at(0).token) + ' ';
        }
        else {
          result += string(children->at(0));
        }
        for (int i = 1; i < children->size(); ++i) {
          result += string(children->at(i));
        }
        return result;
      }
      else if (type == selector_combinator) {
        if (std::isspace(token.begin[0])) return string(" ");
        else return string(" ") += string(token) += string(" ");
      }
      else if (type == simple_selector_sequence) {
        string result;
        for (int i = 0; i < children->size(); ++i) {
          result += string(children->at(i));
        }
        return result;
      }
      else if (type == attribute_selector) {
        string result("[");
        result += string(children->at(0));
        result += string(children->at(1));
        result += string(children->at(2));
        result += ']';
        return result;
      }
      else {
        return string(token);
      }
    }
    
    void release() const { children = 0; }
    
    void echo(stringstream& buf, size_t depth = 0);
    void emit_nested_css(stringstream& buf,
                         size_t depth,
                         const vector<string>& prefixes);
                       void emit_nested_css(stringstream& buf, size_t depth);
    void emit_expanded_css(stringstream& buf, const string& prefix);
  };
}