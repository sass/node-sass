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

  void deprecated(std::string msg, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::cerr << "DEPRECATION WARNING: " << msg << std::endl;
    std::cerr << "will be an error in future versions of Sass." << std::endl;
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::cerr << "        on line " << pstate.line+1 << " of " << rel_path << std::endl;
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
