#define SASS_NODE_INCLUDED

#include <vector>
#include <sstream>
#include "values.hpp"
#include <iostream>

namespace Sass {
  using std::string;
  using std::vector;
  using std::stringstream;
  using std::cerr; using std::endl;

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
      numeric_percentage,
      numeric_dimension,
      number,
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
      assignment,

      comment,
      none,
      flags
    };

    Type type;
    unsigned int line_number;

    bool has_children;
    bool has_statements;
    bool has_blocks;
    bool has_expansions;
    bool has_backref;
    bool from_variable;
    bool eval_me;
    bool unquoted;

    union {
      Token token;
      mutable vector<Node>* children;
      Dimension dimension;
      double numeric_value;
      bool boolean_value;
    } content;
    
    const char* file_name;
    
    static size_t allocations;
    static size_t destructed;
    
    void clear()
    {
      type           = none;  line_number    = 0;     file_name     = 0;
      has_children   = false; has_statements = false; has_blocks    = false;
      has_expansions = false; has_backref    = false; from_variable = false;
      eval_me        = false; unquoted       = false;
    }
    
    size_t size() const
    { return content.children->size(); }
    
    Node& operator[](const size_t i) const
    { return content.children->at(i); }
    
    Node& at(const size_t i) const
    { return content.children->at(i); }
    
    Node& operator<<(const Node& n)
    {
      content.children->push_back(n);
      return *this;
    }
    
    bool is_numeric() const
    {
      switch (type)
      {
        case number:
        case numeric_percentage:
        case numeric_dimension:
          return true;
          break;
        default:
          return false;
      }
    }
    
    double numeric_value() const
    {
      switch (type)
      {
        case number:
        case numeric_percentage:
          return content.numeric_value;
        case numeric_dimension:
          return content.dimension.numeric_value;
        default:
          break;
          // throw an exception?
      }
      return 0;
    }
    
    void set_numeric_value(double v)
    {
        switch (type)
        {
          case number:
          case numeric_percentage:
            content.numeric_value = v;
          case numeric_dimension:
            content.dimension.numeric_value = v;
          default:
            break;
            // throw an exception?
        }
      }
    
    Node& operator+=(const Node& n)
    {
      for (size_t i = 0; i < n.size(); ++i) {
        content.children->push_back(n[i]);
      }
      return *this;
    }
    
    bool operator==(const Node& rhs) const;
    bool operator!=(const Node& rhs) const;
    bool operator<(const Node& rhs) const;
    bool operator<=(const Node& rhs) const;
    bool operator>(const Node& rhs) const;
    bool operator>=(const Node& rhs) const;
    
    string to_string(const string& prefix) const;
        
    void echo(stringstream& buf, size_t depth = 0);
    void emit_nested_css(stringstream& buf,
                         size_t depth,
                         const vector<string>& prefixes);
    void emit_nested_css(stringstream& buf, size_t depth);
    void emit_propset(stringstream& buf, size_t depth, const string& prefix);
    void emit_expanded_css(stringstream& buf, const string& prefix);
    
    Node clone(vector<vector<Node>*>& registry) const;
    void flatten();
    
    Node()
    { clear(); }

    Node(Type t) // flags or booleans
    { clear(); type = t; }

    Node(Type t, vector<vector<Node>*>& registry, unsigned int ln, size_t s = 0) // nodes with children
    {
      clear();
      type = t;
      line_number = ln;
      content.children = new vector<Node>;
      registry.push_back(content.children);
      content.children->reserve(s);
      has_children = true;
      ++allocations;
    }
    
    Node(Type t, unsigned int ln, const Token& tok) // nodes with a single token
    {
      clear();
      type = t;
      line_number = ln;
      content.token = tok;
    }
    
    Node(unsigned int ln, double val) // numeric values
    {
      clear();
      type = number;
      line_number = ln;
      content.numeric_value = val;
    }
    
    Node(unsigned int ln, double val, const Token& tok) // dimensions
    {
      clear();
      type = numeric_dimension;
      line_number = ln;
      content.dimension = Dimension();
      content.dimension.numeric_value = val;
      content.dimension.unit = tok.begin;
    }
    
    Node(vector<vector<Node>*>& registry, unsigned int ln, double r, double g, double b, double a = 1.0) // colors
    {
      clear();
      type = numeric_color;
      line_number = ln;
      content.children = new vector<Node>;
      registry.push_back(content.children);
      content.children->reserve(4);
      content.children->push_back(Node(ln, r));
      content.children->push_back(Node(ln, g));
      content.children->push_back(Node(ln, b));
      content.children->push_back(Node(ln, a));
      has_children = true;
      ++allocations;
    }
    
    ~Node() { ++destructed; }
  };
}
