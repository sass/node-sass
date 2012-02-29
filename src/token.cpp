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
    if (_begin > _end) begin = end = 0;
    else begin = _begin, end = _end;
    line_number = _line_number;
  }
  Token::Token(const Token& t) {
    type = t.type;
    begin = t.begin;
    end = t.end;
    line_number = t.line_number;
  }

}