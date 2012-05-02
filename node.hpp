#define SASS_NODE_INCLUDED

#include <string>
#include <vector>

namespace Sass {
  using namespace std;
  
  struct Node_Impl;

  class Node {
  private:
    friend class Node_Factory;
    Node_Impl* ip_;
    // private constructors; must use a Node_Factory
    Node();
    Node(Node_Impl* ip); // : ip_(ip) { }

  public:
    enum Type {
      none,
      flags,
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

    Type type();           // { return ip_->type; }

    bool has_children();        // { return ip_->has_children; }
    bool has_statements();      // { return ip_->has_statements; }
    bool has_blocks();          // { return ip_->has_blocks; }
    bool has_expansions();      // { return ip_->has_expansions; }
    bool has_backref();         // { return ip_->has_backref; }
    bool from_variable();       // { return ip_->from_variable; }
    bool eval_me();             // { return ip_->eval_me; }
    bool is_unquoted();         // { return ip_->is_unquoted; }
    bool is_numeric();          // { return ip_->is_numeric(); }

    string file_name() const;   // { return *(ip_->file_name); }
    size_t line_number() const; // { return ip_->line_number; }
    size_t size() const;        // { return ip_->size(); }

    Node& at(size_t i) const;         // { return ip_->at(i); }
    Node& operator[](size_t i) const; // { return at(i); }
    void  pop_back();                 // { return ip_->pop_back(); }
    Node& push_back(Node n);
    // { ip_->push_back(n); return *this; }
    Node& operator<<(Node n);
    // { return push_back(n); }
    Node& operator+=(Node n);
    // {
    //       for (size_t i = 0, L = n.size(); i < L; ++i) push_back(n[i]);
    //       return *this;
    // }
    bool   boolean_value();     // { return ip_->boolean_value(); }
    double numeric_value();     // { return ip_->numeric_value(); }
  };
  
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
  
  struct Node_Impl {
    union {
      bool         boolean;
      double       numeric;
      Token        token;
      Dimension    dimension;
    } value;

    vector<Node> children;

    string* file_name;
    size_t line_number;

    Node::Type type;

    bool has_children;
    bool has_statements;
    bool has_blocks;
    bool has_expansions;
    bool has_backref;
    bool from_variable;
    bool eval_me;
    bool is_unquoted;

    // bool is_numeric();
    // 
    // size_t size();
    // Node& at(size_t i);
    // Node& back();
    // Node& pop_back();
    // void push_back(const Node& n);
    // 
    // bool boolean_value();
    
    bool is_numeric()
    { return type >= Node::number && type <= Node::numeric_dimension; }

    size_t size()
    { return children.size(); }

    Node& at(size_t i)
    { return children.at(i); }

    Node& back()
    { return children.back(); }

    void push_back(const Node& n)
    { children.push_back(n); }

    Node& pop_back()
    { children.pop_back(); }

    bool boolean_value()
    { return value.boolean; }
    
    double numeric_value();
    string unit();
  };


  // ------------------------------------------------------------------------
  // Node method implementations
  // -- in the header so they can be easily declared inline
  // -- outside of their class definition to get the right declaration order
  // ------------------------------------------------------------------------
  
  inline Node::Node(Node_Impl* ip) : ip_(ip) { }
  
  inline Node::Type Node::type()          { return ip_->type; }
  
  inline bool Node::has_children()        { return ip_->has_children; }
  inline bool Node::has_statements()      { return ip_->has_statements; }
  inline bool Node::has_blocks()          { return ip_->has_blocks; }
  inline bool Node::has_expansions()      { return ip_->has_expansions; }
  inline bool Node::has_backref()         { return ip_->has_backref; }
  inline bool Node::from_variable()       { return ip_->from_variable; }
  inline bool Node::eval_me()             { return ip_->eval_me; }
  inline bool Node::is_unquoted()         { return ip_->is_unquoted; }
  inline bool Node::is_numeric()          { return ip_->is_numeric(); }
  
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
  inline Node& Node::operator<<(Node n)    { return push_back(n); }
  inline Node& Node::operator+=(Node n)
  {
    for (size_t i = 0, L = n.size(); i < L; ++i) push_back(n[i]);
    return *this;
  }
  inline bool   Node::boolean_value()     { return ip_->boolean_value(); }
  inline double Node::numeric_value()     { return ip_->numeric_value(); }

}