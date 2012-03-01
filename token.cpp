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
}