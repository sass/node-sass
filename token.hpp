#include <string>
#include "prelexer.hpp"

namespace Sass {
  using std::string;
  
  struct Token {
    const char* begin;
    const char* end;

    Token();
    Token(const char* _begin, const char* _end);

    inline bool is_null() { return begin == 0 || end == 0; }
    inline operator string() const { return string(begin, end - begin); }
  };
}