#ifndef SASS_TOKEN_H
#define SASS_TOKEN_H

#include <cstring>
#include <string>
#include <sstream>

#include "position.hpp"

namespace Sass {
  using namespace std;

  // Token type for representing lexed chunks of text
  class Token {
  public:
    const char* begin;
    const char* end;
    Position before;
    Position after;

    Token()
    : begin(0), end(0), before(0), after(0) { }
    Token(const char* b, const char* e, const Position pos)
    : begin(b), end(e), before(pos), after(pos.inc(b, e)) { }
    Token(const char* s, const Position pos)
    : begin(s), end(s + strlen(s)), before(pos), after(pos.inc(s, s + strlen(s))) { }

    size_t length()    const { return end - begin; }
    string to_string() const { return string(begin, end - begin); }

    string unquote() const;
    void   unquote_to_stream(stringstream& buf) const;

    operator bool()   { return begin && end && begin >= end; }
    operator string() { return to_string(); }

    bool operator==(Token t)  { return to_string() == t.to_string(); }
  };

}

#endif
