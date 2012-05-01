#define SASS_NODE_INCLUDED

#include <string>

#ifndef SASS_NODE_TYPE_INCLUDED
#include "node_type.hpp"
#endif

namespace Sass {
  using namespace std;

  class Node_Impl; // forward declaration
  
  class Node {

  private:
    Node_Impl* ip_;
    
  public:
    Node_Type type();
    
    bool has_children();
    bool has_statements();
    bool has_blocks();
    bool has_expansions();
    bool has_backref();
    bool from_variable();
    bool eval_me();
    bool is_unquoted();
    bool is_numeric();
    
    string file_name() const;
    size_t line_number() const;

    size_t size() const;
    Node& at(size_t i) const;
    Node& operator[](size_t i) const;
    Node& pop_back();
    Node& push_back(Node n);
    Node& operator<<(Node n);
    Node& operator+=(Node n);
    
    double numeric_value();
    bool   boolean_value();
  };
}