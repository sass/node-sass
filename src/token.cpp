#include "token.hpp"

namespace Sass {
  Token::Token() {
    begin = 0;
    end = 0;
    line_number = 1;
  }
  Token::Token(Prelexer::prelexer _type,
               const char* _begin,
               const char* _end,
               unsigned int _line_number) {
    type = _type;
    begin = _begin;
    end = _end;
    line_number = _line_number;
  }
}