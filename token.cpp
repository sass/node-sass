#include "token.hpp"

namespace Sass {
  Token::Token() : begin(0), end(0) { }
  Token::Token(const char* _begin, const char* _end)
  : begin(_begin), end(_end) { }
}