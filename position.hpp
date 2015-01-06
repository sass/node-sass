#ifndef SASS_POSITION_H
#define SASS_POSITION_H

#include <string>
#include <cstdlib>
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
      friend ostream& operator<<(ostream& strm, const Offset& off);

    public:
      Offset off() { return *this; };

    public:
      size_t line;
      size_t column;

  };

  class Position : public Offset {

    public: // c-tor
      Position(const size_t file); // line(0), column(0)
      Position(const size_t line, const size_t column); // file(-1)
      Position(const size_t file, const size_t line, const size_t column);

    public: // overload operators for position
      bool operator== (const Position &pos) const;
      bool operator!= (const Position &pos) const;
      const Position operator+ (const Offset &off) const;
      // return new position, incremented by the given string
      Position inc(const char* begin, const char* end) const;

    public: // overload output stream operator
      friend ostream& operator<<(ostream& strm, const Position& pos);

    public:
      size_t file;

  };

  class ParserState : public Position{

    public:
      ParserState(string path);
      ParserState(string path, const size_t file);
      ParserState(string path, Position position, Offset offset);

    public:
      Offset off() { return *this; };
      Position pos() { return *this; };

    public:
      string path;
      Offset offset;

  };

}

#endif
