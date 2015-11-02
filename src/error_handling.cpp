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

  void deprecated_function(std::string msg, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::string abs_path(Sass::File::rel2abs(pstate.path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, abs_path, pstate.path));

    std::cerr << "DEPRECATION WARNING: " << msg << std::endl;
    std::cerr << "will be an error in future versions of Sass." << std::endl;
    std::cerr << "        on line " << pstate.line+1 << " of " << output_path << std::endl;
  }

  void deprecated(std::string msg, std::string msg2, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::string abs_path(Sass::File::rel2abs(pstate.path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, pstate.path, pstate.path));

    std::cerr << "DEPRECATION WARNING on line " << pstate.line + 1;
    if (output_path.length()) std::cerr << " of " << output_path;
    std::cerr << ":" << std::endl;
    std::cerr << msg << " and will be an error in future versions of Sass." << std::endl;
    if (msg2.length()) std::cerr << msg2 << std::endl;
    std::cerr << std::endl;
  }

  void deprecated_bind(std::string msg, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::string abs_path(Sass::File::rel2abs(pstate.path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, abs_path, pstate.path));

    std::cerr << "WARNING: " << msg << std::endl;
    std::cerr << "        on line " << pstate.line+1 << " of " << output_path << std::endl;
    std::cerr << "This will be an error in future versions of Sass." << std::endl;
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
