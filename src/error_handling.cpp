// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include "ast.hpp"
#include "prelexer.hpp"
#include "backtrace.hpp"
#include "error_handling.hpp"

#include <iostream>

namespace Sass {

  namespace Exception {

    Base::Base(ParserState pstate, std::string msg, Backtraces traces)
    : std::runtime_error(msg), msg(msg),
      prefix("Error"), pstate(pstate), traces(traces)
    { }

    InvalidSass::InvalidSass(ParserState pstate, Backtraces traces, std::string msg, char* owned_src)
    : Base(pstate, msg, traces), owned_src(owned_src)
    { }


    InvalidParent::InvalidParent(Selector* parent, Backtraces traces, Selector* selector)
    : Base(selector->pstate(), def_msg, traces), parent(parent), selector(selector)
    {
      msg = "Invalid parent selector for "
        "\"" + selector->to_string(Sass_Inspect_Options()) + "\": "
        "\"" + parent->to_string(Sass_Inspect_Options()) + "\"";
    }

    InvalidVarKwdType::InvalidVarKwdType(ParserState pstate, Backtraces traces, std::string name, const Argument* arg)
    : Base(pstate, def_msg, traces), name(name), arg(arg)
    {
      msg = "Variable keyword argument map must have string keys.\n" +
        name + " is not a string in " + arg->to_string() + ".";
    }

    InvalidArgumentType::InvalidArgumentType(ParserState pstate, Backtraces traces, std::string fn, std::string arg, std::string type, const Value* value)
    : Base(pstate, def_msg, traces), fn(fn), arg(arg), type(type), value(value)
    {
      msg = arg + ": \"";
      if (value) msg += value->to_string(Sass_Inspect_Options());
      msg += "\" is not a " + type + " for `" + fn + "'";
    }

    MissingArgument::MissingArgument(ParserState pstate, Backtraces traces, std::string fn, std::string arg, std::string fntype)
    : Base(pstate, def_msg, traces), fn(fn), arg(arg), fntype(fntype)
    {
      msg = fntype + " " + fn + " is missing argument " + arg + ".";
    }

    InvalidSyntax::InvalidSyntax(ParserState pstate, Backtraces traces, std::string msg)
    : Base(pstate, msg, traces)
    { }

    NestingLimitError::NestingLimitError(ParserState pstate, Backtraces traces, std::string msg)
    : Base(pstate, msg, traces)
    { }

    DuplicateKeyError::DuplicateKeyError(Backtraces traces, const Map& dup, const Expression& org)
    : Base(org.pstate(), def_msg, traces), dup(dup), org(org)
    {
      msg = "Duplicate key " + dup.get_duplicate_key()->inspect() + " in map (" + org.inspect() + ").";
    }

    TypeMismatch::TypeMismatch(Backtraces traces, const Expression& var, const std::string type)
    : Base(var.pstate(), def_msg, traces), var(var), type(type)
    {
      msg = var.to_string() + " is not an " + type + ".";
    }

    InvalidValue::InvalidValue(Backtraces traces, const Expression& val)
    : Base(val.pstate(), def_msg, traces), val(val)
    {
      msg = val.to_string() + " isn't a valid CSS value.";
    }

    StackError::StackError(Backtraces traces, const AST_Node& node)
    : Base(node.pstate(), def_msg, traces), node(node)
    {
      msg = "stack level too deep";
    }

    IncompatibleUnits::IncompatibleUnits(const Units& lhs, const Units& rhs)
    {
      msg = "Incompatible units: '" + rhs.unit() + "' and '" + lhs.unit() + "'.";
    }

    IncompatibleUnits::IncompatibleUnits(const UnitType lhs, const UnitType rhs)
    {
      msg = std::string("Incompatible units: '") + unit_to_string(rhs) + "' and '" + unit_to_string(lhs) + "'.";
    }

    AlphaChannelsNotEqual::AlphaChannelsNotEqual(const Expression* lhs, const Expression* rhs, enum Sass_OP op)
    : OperationError(), lhs(lhs), rhs(rhs), op(op)
    {
      msg = "Alpha channels must be equal: " +
        lhs->to_string({ NESTED, 5 }) +
        " " + sass_op_to_name(op) + " " +
        rhs->to_string({ NESTED, 5 }) + ".";
    }

    ZeroDivisionError::ZeroDivisionError(const Expression& lhs, const Expression& rhs)
    : OperationError(), lhs(lhs), rhs(rhs)
    {
      msg = "divided by 0";
    }

    UndefinedOperation::UndefinedOperation(const Expression* lhs, const Expression* rhs, enum Sass_OP op)
    : OperationError(), lhs(lhs), rhs(rhs), op(op)
    {
      msg = def_op_msg + ": \"" +
        lhs->to_string({ NESTED, 5 }) +
        " " + sass_op_to_name(op) + " " +
        rhs->to_string({ TO_SASS, 5 }) +
        "\".";
    }

    InvalidNullOperation::InvalidNullOperation(const Expression* lhs, const Expression* rhs, enum Sass_OP op)
    : UndefinedOperation(lhs, rhs, op)
    {
      msg = def_op_null_msg + ": \"" + lhs->inspect() + " " + sass_op_to_name(op) + " " + rhs->inspect() + "\".";
    }

    SassValueError::SassValueError(Backtraces traces, ParserState pstate, OperationError& err)
    : Base(pstate, err.what(), traces)
    {
      msg = err.what();
      prefix = err.errtype();
    }

  }


  void warn(std::string msg, ParserState pstate)
  {
    std::cerr << "Warning: " << msg << std::endl;
  }

  void warning(std::string msg, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::string abs_path(Sass::File::rel2abs(pstate.path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, abs_path, pstate.path));

    std::cerr << "WARNING on line " << pstate.line+1 << ", column " << pstate.column+1 << " of " << output_path << ":" << std::endl;
    std::cerr << msg << std::endl << std::endl;
  }

  void warn(std::string msg, ParserState pstate, Backtrace* bt)
  {
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

  void deprecated(std::string msg, std::string msg2, bool with_column, ParserState pstate)
  {
    std::string cwd(Sass::File::get_cwd());
    std::string abs_path(Sass::File::rel2abs(pstate.path, cwd, cwd));
    std::string rel_path(Sass::File::abs2rel(pstate.path, cwd, cwd));
    std::string output_path(Sass::File::path_for_console(rel_path, pstate.path, pstate.path));

    std::cerr << "DEPRECATION WARNING on line " << pstate.line + 1;
    if (with_column) std::cerr << ", column " << pstate.column + pstate.offset.column + 1;
    if (output_path.length()) std::cerr << " of " << output_path;
    std::cerr << ":" << std::endl;
    std::cerr << msg << std::endl;
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

  // should be replaced with error with backtraces
  void coreError(std::string msg, ParserState pstate)
  {
    Backtraces traces;
    throw Exception::InvalidSyntax(pstate, traces, msg);
  }

  void error(std::string msg, ParserState pstate, Backtraces& traces)
  {
    traces.push_back(Backtrace(pstate));
    throw Exception::InvalidSyntax(pstate, traces, msg);
  }

}
