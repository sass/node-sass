#include <string>
#include "prelexer.hpp"

namespace Sass {
  using std::string;
  
  struct Token {
    Prelexer::prelexer type;
    const char* begin;
    const char* end;
    unsigned int line_number;
    Token();
    Token(Prelexer::prelexer _type,
          const char* _begin,
          const char* _end,
          unsigned int _line_number);
    Token(const Token& t);
    inline bool is_null() { return begin == 0 || end == 0; }
    inline operator string() const { return string(begin, end - begin); }
    inline bool operator<(const Token& rhs) const {
      return (begin < rhs.begin) ||
             (begin == rhs.begin && end < rhs.end);
    }
             
  };
}