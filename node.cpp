#include <sstream>
#include "node.hpp"

namespace Sass {
  using namespace std;
  
  // Node::Node(Node_Impl* ip) : ip_(ip) { }
  // 
  // inline Node_Type Node::type()           { return ip_->type; }
  // 
  // inline bool Node::has_children()        { return ip_->has_children; }
  // inline bool Node::has_statements()      { return ip_->has_statements; }
  // inline bool Node::has_blocks()          { return ip_->has_blocks; }
  // inline bool Node::has_expansions()      { return ip_->has_expansions; }
  // inline bool Node::has_backref()         { return ip_->has_backref; }
  // inline bool Node::from_variable()       { return ip_->from_variable; }
  // inline bool Node::eval_me()             { return ip_->eval_me; }
  // inline bool Node::is_unquoted()         { return ip_->is_unquoted; }
  // inline bool Node::is_numeric()          { return ip_->is_numeric(); }
  // 
  // inline string Node::file_name() const   { return *(ip_->file_name); }
  // inline size_t Node::line_number() const { return ip_->line_number; }
  // inline size_t Node::size() const        { return ip_->size(); }
  // 
  // inline Node& Node::at(size_t i) const         { return ip_->at(i); }
  // inline Node& Node::operator[](size_t i) const { return at(i); }
  // inline Node& Node::pop_back()            { return ip_->pop_back(); }
  // inline Node& Node::push_back(Node n)
  // {
  //   ip_->push_back(n);
  //   return *this;
  // }
  // inline Node& Node::operator<<(Node n)    { return push_back(n); }
  // inline Node& Node::operator+=(Node n)
  // {
  //   for (size_t i = 0, L = n.size(); i < L; ++i) push_back(n[i]);
  //   return *this;
  // }
  // inline bool   Node::boolean_value()     { return ip_->boolean_value(); }
  // inline double Node::numeric_value()     { return ip_->numeric_value(); }
  
  // ------------------------------------------------------------------------
  // Token method implementations
  // ------------------------------------------------------------------------
  
  string Token::unquote() const
  {
    string result;
    const char* p = begin;
    if (*begin == '\'' || *begin == '"') {
      ++p;
      while (p < end) {
        if (*p == '\\') {
          switch (*(++p)) {
            case 'n':  result += '\n'; break;
            case 't':  result += '\t'; break;
            case 'b':  result += '\b'; break;
            case 'r':  result += '\r'; break;
            case 'f':  result += '\f'; break;
            case 'v':  result += '\v'; break;
            case 'a':  result += '\a'; break;
            case '\\': result += '\\'; break;
            default: result += *p; break;
          }
        }
        else if (p == end - 1) {
          return result;
        }
        else {
          result += *p;
        }
        ++p;
      }
      return result;
    }
    else {
      while (p < end) {
        result += *(p++);
      }
      return result;
    }
  }
  
  void Token::unquote_to_stream(std::stringstream& buf) const
  {
    const char* p = begin;
    if (*begin == '\'' || *begin == '"') {
      ++p;
      while (p < end) {
        if (*p == '\\') {
          switch (*(++p)) {
            case 'n':  buf << '\n'; break;
            case 't':  buf << '\t'; break;
            case 'b':  buf << '\b'; break;
            case 'r':  buf << '\r'; break;
            case 'f':  buf << '\f'; break;
            case 'v':  buf << '\v'; break;
            case 'a':  buf << '\a'; break;
            case '\\': buf << '\\'; break;
            default: buf << *p; break;
          }
        }
        else if (p == end - 1) {
          return;
        }
        else {
          buf << *p;
        }
        ++p;
      }
      return;
    }
    else {
      while (p < end) {
        buf << *(p++);
      }
      return;
    }
  }
  
  bool Token::operator<(const Token& rhs) const
  {
    const char* first1 = begin;
    const char* last1  = end;
    const char* first2 = rhs.begin;
    const char* last2  = rhs.end;
    while (first1!=last1)
    {
      if (first2==last2 || *first2<*first1) return false;
      else if (*first1<*first2) return true;
      first1++; first2++;
    }
    return (first2!=last2);
  }
  
  bool Token::operator==(const Token& rhs) const
  {
    if (length() != rhs.length()) return false;
    
    if ((begin[0]     == '"' || begin[0]     == '\'') &&
        (rhs.begin[0] == '"' || rhs.begin[0] == '\''))
    { return unquote() == rhs.unquote(); }
    
    const char* p = begin;
    const char* q = rhs.begin;
    for (; p < end; ++p, ++q) if (*p != *q) return false;
    return true;
  }


  // ------------------------------------------------------------------------
  // Node_Impl method implementations
  // ------------------------------------------------------------------------

  // inline bool Node_Impl::is_numeric()
  // { return type >= number && type <= numeric_dimension; }
  // 
  // inline size_t Node_Impl::size()
  // { return children.size(); }
  // 
  // inline Node& Node_Impl::at(size_t i)
  // { return children.at(i); }
  // 
  // inline Node& Node_Impl::back()
  // { return children.back(); }
  // 
  // inline void Node_Impl::push_back(const Node& n)
  // { children.push_back(n); }
  // 
  // inline Node& Node_Impl::pop_back()
  // { children.pop_back(); }
  // 
  // inline bool Node_Impl::boolean_value()
  // { return value.boolean; }
  // 
  double Node_Impl::numeric_value()
  {
    switch (type)
    {
      case Node::number:
      case Node::numeric_percentage:
        return value.numeric;
      case Node::numeric_dimension:
        return value.dimension.numeric;
      default:
        break;
        // throw an exception?
    }
    return 0;
  }
  
  string Node_Impl::unit()
  {
    switch (type)
    {
      case Node::numeric_percentage: {
        return "\"%\"";
      } break;
  
      case Node::numeric_dimension: {
        string result("\"");
        result += value.dimension.unit.to_string();
        result += "\"";
        return result;
      } break;
      
      default: break;
    }
    return "\"\"";
  }

}