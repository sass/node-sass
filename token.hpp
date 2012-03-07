#include <string>
#include <sstream>
#include "prelexer.hpp"

namespace Sass {
  using std::string;
  
  struct Token {
    const char* begin;
    const char* end;

    Token();
    Token(const char* _begin, const char* _end);

    inline bool is_null() const {
      return begin == 0 || end == 0 || begin >= end;
    }
    inline operator string() const {
      return string(begin, end - begin);
    }

    void stream_unquoted(std::stringstream& buf) const;
    
    bool operator<(const Token& rhs) const;
    
  };
}