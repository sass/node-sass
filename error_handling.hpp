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
    Position position;
    string message;

    Sass_Error(Type type, string path, Position position, string message);

  };

  void error(string msg, string path, Position position);
  void error(string msg, string path, Position position, Backtrace* bt);

}
