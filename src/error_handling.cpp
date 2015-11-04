#include "ast.hpp"
#include "prelexer.hpp"
#include "backtrace.hpp"
#include "to_string.hpp"
#include "error_handling.hpp"

#include <iostream>

namespace Sass {

  namespace Exception {

    Base::Base(ParserState pstate, std::string msg)
    : std::runtime_error(msg),
      msg(msg), pstate(pstate)
    { }

    const char* Base::what() const throw()
    {
      return msg.c_str();
    }

    InvalidSass::InvalidSass(ParserState pstate, std::string msg)
    : Base(pstate, msg)
    { }


    InvalidParent::InvalidParent(Selector* parent, Selector* selector)
    : Base(selector->pstate()), parent(parent), selector(selector)
    {
      msg = "Invalid parent selector for \"";
      msg += selector->to_string(false);
      msg += "\": \"";
      msg += parent->to_string(false);;
      msg += "\"";
    }

    InvalidArgumentType::InvalidArgumentType(ParserState pstate, std::string fn, std::string arg, std::string type, const Value* value)
    : Base(pstate), fn(fn), arg(arg), type(type), value(value)
    {
      msg  = arg + ": \"";
      msg += value->to_string(true, 5);
      msg += "\" is not a " + type;
      msg += " for `" + fn + "'";
    }

    InvalidSyntax::InvalidSyntax(ParserState pstate, std::string msg)
    : Base(pstate, msg)
    { }

  }


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
    throw Exception::InvalidSyntax(pstate, msg);
  }

  void error(std::string msg, ParserState pstate, Backtrace* bt)
  {
    Backtrace top(bt, pstate, "");
    msg += "\n" + top.to_string();
    error(msg, pstate);
  }

}
