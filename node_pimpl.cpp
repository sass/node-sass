#ifndef SASS_NODE_INCLUDED
#include "node_pimpl.hpp"
#endif

#include "node_impl.hpp"

namespace Sass {
  using namespace std;

  inline Node_Type Node::type()           { return ip_->type; }
  
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
  inline Node& Node::pop_back()            { return ip_->pop_back(); }
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