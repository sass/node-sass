#include <cstring>
#include "node_impl.hpp"

namespace Sass {
  
  // ------------------------------------------------------------------------
  // Token method implementations
  // ------------------------------------------------------------------------
  
  inline size_t Token::length() const
  { return end - begin; }
  
  inline string Token::to_string() const
  { return string(begin, end - begin); }
  
  inline Token::operator bool()
  { return begin && end && begin >= end; }
  
  // Need Token::make(...) because tokens are union members, and hence they
  // can't have user-implemented default and copy constructors.
  inline Token Token::make()
  {
    Token t;
    t.begin = 0;
    t.end = 0;
    return t;
  }
  
  inline Token Token::make(const char* s)
  {
    Token t;
    t.begin = s;
    t.end = s + std::strlen(s);
    return t;
  }
  
  inline Token Token::make(const char* b, const char* e)
  {
    Token t;
    t.begin = b;
    t.end = e;
    return t;
  }
  
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

  inline bool Node_Impl::is_numeric()
  { return type >= number && type <= numeric_color; }

  inline size_t Node_Impl::size()
  { return children.size(); }

  inline Node& Node_Impl::at(size_t i)
  { return children.at(i); }
  
  inline Node& Node_Impl::back()
  { return children.back(); }
  
  inline void Node_Impl::push_back(const Node& n)
  { children.push_back(n); }
  
  inline Node& Node_Impl::pop_back()
  { children.pop_back(); }
  
  inline bool Node_Impl::boolean_value()
  { return value.boolean; }
  
  inline double Node_Impl::numeric_value()
  {
    switch (type)
    {
      case number:
      case numeric_percentage:
        return value.numeric;
      case numeric_dimension:
        return value.dimension.numeric_value;
      default:
        break;
        // throw an exception?
    }
    return 0;
  }
  
  inline string Node_Impl::unit()
  {
    switch (type)
    {
      case numeric_percentage: {
        return "\"%\"";
      } break;

      case numeric_dimension: {
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