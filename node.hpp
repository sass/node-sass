#define SASS_NODE_INCLUDED

#include <cstring>
#include <string>
#include <vector>

namespace Sass {
  using namespace std;

  struct Token {
    const char* begin;
    const char* end;

    // Need Token::make(...) because tokens are union members, and hence they
    // can't have non-trivial constructors.
    static Token make()
    {
      Token t;
      t.begin = 0;
      t.end = 0;
      return t;
    }

    static Token make(const char* s)
    {
      Token t;
      t.begin = s;
      t.end = s + std::strlen(s);
      return t;
    }

    static Token make(const char* b, const char* e)
    {
      Token t;
      t.begin = b;
      t.end = e;
      return t;
    }

    size_t length() const
    { return end - begin; }

    string to_string() const
    { return string(begin, end - begin); }

    string unquote() const;
    void   unquote_to_stream(std::stringstream& buf) const;
    
    operator bool()
    { return begin && end && begin >= end; }

    bool operator<(const Token& rhs) const;
    bool operator==(const Token& rhs) const;
  };

  struct Dimension {
    double numeric;
    Token unit;
  };
  
  struct Node_Impl;

  class Node {
  private:
    friend class Node_Factory;
    Node_Impl* ip_;
    // private constructors; must use a Node_Factory
    Node();
    Node(Node_Impl* ip);

  public:
    enum Type {
      none,
      comment,

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

      disjunction,
      conjunction,

      relation,
      eq,
      neq,
      gt,
      gte,
      lt,
      lte,

      expression,
      add,
      sub,

      term,
      mul,
      div,

      factor,
      unary_plus,
      unary_minus,
      values,
      value,
      identifier,
      uri,
      textual_percentage,
      textual_dimension,
      textual_number,
      textual_hex,
      color_name,
      string_constant,
      number,
      numeric_percentage,
      numeric_dimension,
      numeric_color,
      boolean,
      important,

      value_schema,
      string_schema,

      css_import,
      function_call,
      mixin,
      parameters,
      expansion,
      arguments,

      variable,
      assignment
    };

    Type type() const;

    bool has_children() const;
    bool has_statements() const;
    bool has_blocks() const;
    bool has_expansions() const;
    bool has_backref() const;
    bool from_variable() const;
    bool should_eval() const;
    bool is_unquoted() const;
    bool is_numeric() const;

    string file_name() const;
    size_t line_number() const;
    size_t size() const;

    Node& at(size_t i) const;
    Node& operator[](size_t i) const;
    void  pop_back();
    Node& push_back(Node n);
    Node& operator<<(Node n);
    Node& operator+=(Node n);

    bool   boolean_value() const;
    double numeric_value() const;
    Token  token() const;
    Token  unit() const;
    
    bool operator==(Node rhs) const;
    bool operator!=(Node rhs) const;
    bool operator<(Node rhs) const;
    bool operator<=(Node rhs) const;
    bool operator>(Node rhs) const;
    bool operator>=(Node rhs) const;
  };
  
  struct Node_Impl {
    union {
      bool         boolean;
      double       numeric;
      Token        token;
      Dimension    dimension;
    } value;

    // TO DO: look into using a custom allocator in the Node_Factory class
    vector<Node> children; // Can't be in the union because it has non-trivial constructors!

    string* file_name;
    size_t  line_number;

    Node::Type type;

    bool has_children;
    bool has_statements;
    bool has_blocks;
    bool has_expansions;
    bool has_backref;
    bool from_variable;
    bool should_eval;
    bool is_unquoted;
    
    bool is_numeric()
    { return type >= Node::number && type <= Node::numeric_dimension; }

    size_t size()
    { return children.size(); }
    
    bool empty()
    { return children.empty(); }

    Node& at(size_t i)
    { return children.at(i); }

    Node& back()
    { return children.back(); }

    void push_back(const Node& n)
    {
      children.push_back(n);
      has_children = true;
      switch (type)
      {
        case Node::comment:
        case Node::css_import:
        case Node::rule:
        case Node::propset:  has_statements = true; break;
        case Node::ruleset:   has_blocks     = true; break;
        case Node::expansion: has_expansions = true; break;
        default:                               break;
      }
    }

    void pop_back()
    { children.pop_back(); }

    bool boolean_value()
    { return value.boolean; }
    
    double numeric_value();
    Token  unit();
  };


  // ------------------------------------------------------------------------
  // Node method implementations
  // -- in the header file so they can easily be declared inline
  // -- outside of their class definition to get the right declaration order
  // ------------------------------------------------------------------------
  
  inline Node::Node(Node_Impl* ip) : ip_(ip) { }
  
  inline Node::Type Node::type() const    { return ip_->type; }
  
  inline bool Node::has_children() const   { return ip_->has_children; }
  inline bool Node::has_statements() const { return ip_->has_statements; }
  inline bool Node::has_blocks() const     { return ip_->has_blocks; }
  inline bool Node::has_expansions() const { return ip_->has_expansions; }
  inline bool Node::has_backref() const    { return ip_->has_backref; }
  inline bool Node::from_variable() const  { return ip_->from_variable; }
  inline bool Node::should_eval() const    { return ip_->should_eval; }
  inline bool Node::is_unquoted() const    { return ip_->is_unquoted; }
  inline bool Node::is_numeric() const     { return ip_->is_numeric(); }
  
  inline string Node::file_name() const   { return *(ip_->file_name); }
  inline size_t Node::line_number() const { return ip_->line_number; }
  inline size_t Node::size() const        { return ip_->size(); }
  
  inline Node& Node::at(size_t i) const         { return ip_->at(i); }
  inline Node& Node::operator[](size_t i) const { return at(i); }
  inline void  Node::pop_back()                 { ip_->pop_back(); }
  inline Node& Node::push_back(Node n)
  {
    ip_->push_back(n);
    return *this;
  }
  inline Node& Node::operator<<(Node n)         { return push_back(n); }
  inline Node& Node::operator+=(Node n)
  {
    for (size_t i = 0, L = n.size(); i < L; ++i) push_back(n[i]);
    return *this;
  }
  inline bool   Node::boolean_value() const { return ip_->boolean_value(); }
  inline double Node::numeric_value() const { return ip_->numeric_value(); }
  inline Token  Node::token() const         { return ip_->value.token; }
  inline Token  Node::unit() const          { return ip_->unit(); }

}