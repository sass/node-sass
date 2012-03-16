#include <string>
#include <sstream>
#include "prelexer.hpp"

namespace Sass {
  using std::string;
  
  struct Token {
    const char* begin;
    const char* end;

    Token();
    Token(const char* begin, const char* end);

    inline operator string() const
    { return string(begin, end - begin); }

    string unquote() const;
    void unquote_to_stream(std::stringstream& buf) const;
    
    bool operator<(const Token& rhs) const;
    
    operator bool()
    { return begin && end && begin >= end; }
    
  };
}