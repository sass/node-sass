#include "token.hpp"

namespace Sass {
  Token::Token() : begin(0), end(0) { }
  Token::Token(const char* _begin, const char* _end)
  : begin(_begin), end(_end) { }
  
  void Token::stream_unquoted(std::stringstream& buf) const {
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
  
  using std::lexicographical_compare;
  bool Token::operator<(const Token& rhs) const
  {
    const char* first1 = begin;
    const char* last1 = end;
    const char* first2 = rhs.begin;
    const char* last2 = rhs.end;
    while (first1!=last1)
    {
      if (first2==last2 || *first2<*first1) return false;
      else if (*first1<*first2) return true;
      first1++; first2++;
    }
    return (first2!=last2);
  }
}