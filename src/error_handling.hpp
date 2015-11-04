#ifndef SASS_ERROR_HANDLING_H
#define SASS_ERROR_HANDLING_H

#include <string>
#include <sstream>
#include <stdexcept>
#include "position.hpp"

namespace Sass {

  struct Backtrace;

  namespace Exception {

    const std::string def_msg = "Invalid sass";

    class Base : public std::runtime_error {
      protected:
        std::string msg;
      public:
        ParserState pstate;
      public:
        Base(ParserState pstate, std::string msg = def_msg);
        virtual const char* what() const throw();
        virtual ~Base() throw() {};
    };

    class InvalidSass : public Base {
      public:
        InvalidSass(ParserState pstate, std::string msg);
        virtual ~InvalidSass() throw() {};
    };

    class InvalidParent : public Base {
      protected:
        Selector* parent;
        Selector* selector;
      public:
        InvalidParent(Selector* parent, Selector* selector);
        virtual ~InvalidParent() throw() {};
    };

    class InvalidArgumentType : public Base {
      protected:
        std::string fn;
        std::string arg;
        std::string type;
        const Value* value;
      public:
        InvalidArgumentType(ParserState pstate, std::string fn, std::string arg, std::string type, const Value* value = 0);
        virtual ~InvalidArgumentType() throw() {};
    };

    class InvalidSyntax : public Base {
      public:
        InvalidSyntax(ParserState pstate, std::string msg);
        virtual ~InvalidSyntax() throw() {};
    };

  }

  void warn(std::string msg, ParserState pstate);
  void warn(std::string msg, ParserState pstate, Backtrace* bt);

  void deprecated_function(std::string msg, ParserState pstate);
  void deprecated(std::string msg, std::string msg2, ParserState pstate);
  void deprecated_bind(std::string msg, ParserState pstate);
  // void deprecated(std::string msg, ParserState pstate, Backtrace* bt);

  void error(std::string msg, ParserState pstate);
  void error(std::string msg, ParserState pstate, Backtrace* bt);

}

#endif
