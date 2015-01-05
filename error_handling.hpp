#define SASS_ERROR_HANDLING
#include <string>

#ifndef SASS_POSITION
#include "position.hpp"
#endif

namespace Sass {
  using namespace std;

  struct Backtrace;

  struct Sass_Error {
    enum Type { read, write, syntax, evaluation };

    Type type;
    string path;
    ParserState pstate;
    string message;

    Sass_Error(Type type, ParserState pstate, string message);

  };

  void error(string msg, ParserState pstate);
  void error(string msg, ParserState pstate, Backtrace* bt);

}
