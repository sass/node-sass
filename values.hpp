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
    
    size_t length() const
    { return end - begin; }

    inline operator string() const
    { return string(begin, end - begin); }

    string unquote() const;
    void unquote_to_stream(std::stringstream& buf) const;
    
    bool operator<(const Token& rhs) const;
    bool operator==(const Token& rhs) const;
    
    operator bool()
    { return begin && end && begin >= end; }  
  };
  
  struct Dimension {
    const double numeric_value;
    const char* unit;
  };
}