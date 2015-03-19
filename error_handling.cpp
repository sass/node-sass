#include "prelexer.hpp"
#include "backtrace.hpp"
#include "error_handling.hpp"

namespace Sass {

  Sass_Error::Sass_Error(Type type, ParserState pstate, string message)
  : type(type), pstate(pstate), message(message)
  { }

  void error(string msg, ParserState pstate)
  { throw Sass_Error(Sass_Error::syntax, pstate, msg); }

  void error(string msg, ParserState pstate, Backtrace* bt)
  {

    Backtrace top(bt, pstate, "");
    msg += top.to_string();

    throw Sass_Error(Sass_Error::syntax, pstate, msg);
  }

}
