#ifndef SASS_ERROR_HANDLING_H
#define SASS_ERROR_HANDLING_H

#include <string>

#include "position.hpp"

namespace Sass {

  struct Backtrace;

  struct Error_Invalid {
    enum Type { read, write, syntax, evaluation };

    Type type;
    ParserState pstate;
    std::string message;

    Error_Invalid(Type type, ParserState pstate, std::string message);

  };

  void warn(std::string msg, ParserState pstate);
  void warn(std::string msg, ParserState pstate, Backtrace* bt);

  void deprecated(std::string msg, ParserState pstate);
  // void deprecated(std::string msg, ParserState pstate, Backtrace* bt);

  void error(std::string msg, ParserState pstate);
  void error(std::string msg, ParserState pstate, Backtrace* bt);

}

#endif
