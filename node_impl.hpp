#include <vector>
#include <sstream>

#ifndef SASS_NODE_TYPE_INCLUDED
#include "node_type.hpp"
#endif

#ifndef SASS_NODE_INCLUDED
#include "node_pimpl.hpp"
#endif

namespace Sass {
  using namespace std;
  
  struct Token {
    const char* begin;
    const char* end;

    // Need Token::make(...) because tokens are union members, and hence they
    // can't have user-implemented default and copy constructors.
    static Token make();
    static Token make(const char* s);
    static Token make(const char* b, const char* e);

    size_t length() const;

    string to_string() const;
    string unquote() const;
    void   unquote_to_stream(std::stringstream& buf) const;

    bool operator<(const Token& rhs) const;
    bool operator==(const Token& rhs) const;
    operator bool();
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

    Node_Type type;

    bool has_children;
    bool has_statements;
    bool has_blocks;
    bool has_expansions;
    bool has_backref;
    bool from_variable;
    bool eval_me;
    bool is_unquoted;

    bool is_numeric();
    
    size_t size();
    Node& at(size_t i);
    Node& back();
    Node& pop_back();
    void push_back(const Node& n);
    
    bool boolean_value();
    double numeric_value();
    string unit();
  };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
}