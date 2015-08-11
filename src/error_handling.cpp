#include "prelexer.hpp"
#include "backtrace.hpp"
#include "error_handling.hpp"

#include <iostream>

namespace Sass {

  Error_Invalid::Error_Invalid(Type type, ParserState pstate, std::string message)
  : type(type), pstate(pstate), message(message)
  { }

  void warn(std::string msg, ParserState pstate)
  {
    std::cerr << "Warning: " << msg<< std::endl;
  }

  void warn(std::string msg, ParserState pstate, Backtrace* bt)
  {
    Backtrace top(bt, pstate, "");
    msg += top.to_string();
    warn(msg, pstate);
  }

  void deprecated(std::string msg, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::cerr << "DEPRECATION WARNING: " << msg << std::endl;
    std::cerr << "will be an error in future versions of Sass." << std::endl;
    std::string rel_path(Sass::File::resolve_relative_path(pstate.path, cwd, cwd));
    std::cerr << "        on line " << pstate.line+1 << " of " << rel_path << std::endl;
  }

  void error(std::string msg, ParserState pstate)
  {
    throw Error_Invalid(Error_Invalid::syntax, pstate, msg);
  }

  void error(std::string msg, ParserState pstate, Backtrace* bt)
  {
    Backtrace top(bt, pstate, "");
    msg += "\n" + top.to_string();
    error(msg, pstate);
  }

}
