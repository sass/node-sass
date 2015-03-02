#ifndef SASS_POSITION_H
#define SASS_POSITION_H

#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>

namespace Sass {

  using namespace std;

  class Offset {

    public: // c-tor
      Offset(const size_t line, const size_t column);

      // return new position, incremented by the given string
      Offset inc(const char* begin, const char* end) const;

    public: // overload operators for position
      bool operator== (const Offset &pos) const;
      bool operator!= (const Offset &pos) const;
      const Offset operator+ (const Offset &off) const;

    public: // overload output stream operator
      // friend ostream& operator<<(ostream& strm, const Offset& off);

    public:
      Offset off() { return *this; };

    public:
      size_t line;
      size_t column;

  };

  class Position : public Offset {

    public: // c-tor
      Position(const size_t file); // line(0), column(0)
      Position(const size_t file, const Offset& offset);
      Position(const size_t line, const size_t column); // file(-1)
      Position(const size_t file, const size_t line, const size_t column);

    public: // overload operators for position
      bool operator== (const Position &pos) const;
      bool operator!= (const Position &pos) const;
      const Position operator+ (const Offset &off) const;
      // return new position, incremented by the given string
      Position inc(const char* begin, const char* end) const;

    public: // overload output stream operator
      // friend ostream& operator<<(ostream& strm, const Position& pos);

    public:
      size_t file;

  };

  // Token type for representing lexed chunks of text
  class Token {
  public:
    const char* prefix;
    const char* begin;
    const char* end;
    const char* suffix;
    Position start;
    Position stop;

    Token()
    : prefix(0), begin(0), end(0), suffix(0), start(0), stop(0) { }
    Token(const char* b, const char* e, const Position pos)
    : prefix(b), begin(b), end(e), suffix(e), start(pos), stop(pos.inc(b, e)) { }
    Token(const char* s, const Position pos)
    : prefix(s), begin(s), end(s + strlen(s)), suffix(end), start(pos), stop(pos.inc(s, s + strlen(s))) { }
    Token(const char* p, const char* b, const char* e, const char* s, const Position pos)
    : prefix(p), begin(b), end(e), suffix(s), start(pos), stop(pos.inc(b, e)) { }

    size_t length()    const { return end - begin; }
    string ws_before() const { return string(prefix, begin); }
    string to_string() const { return string(begin, end); }
    string ws_after() const { return string(end, suffix); }

    // string unquote() const;

    operator bool()   { return begin && end && begin >= end; }
    operator string() { return to_string(); }

    bool operator==(Token t)  { return to_string() == t.to_string(); }
  };

  class ParserState : public Position {

    public: // c-tor
      ParserState(string path);
      ParserState(string path, const size_t file);
      ParserState(string path, Position position, Offset offset);
      ParserState(string path, Token token, Position position, Offset offset);

    public: // down casts
      Offset off() { return *this; };
      Position pos() { return *this; };

    public:
      string path;
      Offset offset;
      Token token;

  };

}

#endif
