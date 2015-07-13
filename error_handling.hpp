#ifndef SASS_ERROR_HANDLING_H
#define SASS_ERROR_HANDLING_H

#include <string>

#include "position.hpp"

namespace Sass {
  using namespace std;

  struct Backtrace;

  struct Error_Invalid {
    enum Type { read, write, syntax, evaluation };

    Type type;
    ParserState pstate;
    string message;

    Error_Invalid(Type type, ParserState pstate, string message);

  };

  void warn(string msg, ParserState pstate);
  void warn(string msg, ParserState pstate, Backtrace* bt);

  void deprecated(string msg, ParserState pstate);
  // void deprecated(string msg, ParserState pstate, Backtrace* bt);

  void error(string msg, ParserState pstate);
  void error(string msg, ParserState pstate, Backtrace* bt);

}

#endif