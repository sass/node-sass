// sass.hpp must go before all system headers to get the
// __EXTENSIONS__ fix on Solaris.
#include "sass.hpp"

#include "parser.hpp"
#include "fn_utils.hpp"
#include "util_string.hpp"

namespace Sass {

  Definition* make_native_function(Signature sig, Native_Function func, Context& ctx)
  {
    Parser sig_parser = Parser::from_c_str(sig, ctx, ctx.traces, ParserState("[built-in function]"));
    sig_parser.lex<Prelexer::identifier>();
    std::string name(Util::normalize_underscores(sig_parser.lexed));
    Parameters_Obj params = sig_parser.parse_parameters();
    return SASS_MEMORY_NEW(Definition,
                          ParserState("[built-in function]"),
                          sig,
                          name,
                          params,
                          func,
                          false);
  }

  Definition* make_c_function(Sass_Function_Entry c_func, Context& ctx)
  {
    using namespace Prelexer;

    const char* sig = sass_function_get_signature(c_func);
    Parser sig_parser = Parser::from_c_str(sig, ctx, ctx.traces, ParserState("[c function]"));
    // allow to overload generic callback plus @warn, @error and @debug with custom functions
    sig_parser.lex < alternatives < identifier, exactly <'*'>,
                                    exactly < Constants::warn_kwd >,
                                    exactly < Constants::error_kwd >,
                                    exactly < Constants::debug_kwd >
                  >              >();
    std::string name(Util::normalize_underscores(sig_parser.lexed));
    Parameters_Obj params = sig_parser.parse_parameters();
    return SASS_MEMORY_NEW(Definition,
                          ParserState("[c function]"),
                          sig,
                          name,
                          params,
                          c_func);
  }

  namespace Functions {

    std::string function_name(Signature sig)
    {
      std::string str(sig);
      return str.substr(0, str.find('('));
    }

    Map* get_arg_m(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces)
    {
      AST_Node* value = env[argname];
      if (Map* map = Cast<Map>(value)) return map;
      List* list = Cast<List>(value);
      if (list && list->length() == 0) {
        return SASS_MEMORY_NEW(Map, pstate, 0);
      }
      return get_arg<Map>(argname, env, sig, pstate, traces);
    }

    double get_arg_r(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, double lo, double hi)
    {
      Number* val = get_arg<Number>(argname, env, sig, pstate, traces);
      Number tmpnr(val);
      tmpnr.reduce();
      double v = tmpnr.value();
      if (!(lo <= v && v <= hi)) {
        std::stringstream msg;
        msg << "argument `" << argname << "` of `" << sig << "` must be between ";
        msg << lo << " and " << hi;
        error(msg.str(), pstate, traces);
      }
      return v;
    }

    Number* get_arg_n(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces)
    {
      Number* val = get_arg<Number>(argname, env, sig, pstate, traces);
      val = SASS_MEMORY_COPY(val);
      val->reduce();
      return val;
    }

    double get_arg_val(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces)
    {
      Number* val = get_arg<Number>(argname, env, sig, pstate, traces);
      Number tmpnr(val);
      tmpnr.reduce();
      return tmpnr.value();
    }

    double color_num(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces)
    {
      Number* val = get_arg<Number>(argname, env, sig, pstate, traces);
      Number tmpnr(val);
      tmpnr.reduce();
      if (tmpnr.unit() == "%") {
        return std::min(std::max(tmpnr.value() * 255 / 100.0, 0.0), 255.0);
      } else {
        return std::min(std::max(tmpnr.value(), 0.0), 255.0);
      }
    }

    double alpha_num(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces) {
      Number* val = get_arg<Number>(argname, env, sig, pstate, traces);
      Number tmpnr(val);
      tmpnr.reduce();
      if (tmpnr.unit() == "%") {
        return std::min(std::max(tmpnr.value(), 0.0), 100.0);
      } else {
        return std::min(std::max(tmpnr.value(), 0.0), 1.0);
      }
    }

    SelectorListObj get_arg_sels(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, Context& ctx) {
      Expression_Obj exp = ARG(argname, Expression);
      if (exp->concrete_type() == Expression::NULL_VAL) {
        std::stringstream msg;
        msg << argname << ": null is not a valid selector: it must be a string,\n";
        msg << "a list of strings, or a list of lists of strings for `" << function_name(sig) << "'";
        error(msg.str(), exp->pstate(), traces);
      }
      if (String_Constant* str = Cast<String_Constant>(exp)) {
        str->quote_mark(0);
      }
      std::string exp_src = exp->to_string(ctx.c_options);
      return Parser::parse_selector(exp_src.c_str(), ctx, traces, exp->pstate(), pstate.src, /*allow_parent=*/false);
    }

    CompoundSelectorObj get_arg_sel(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, Context& ctx) {
      Expression_Obj exp = ARG(argname, Expression);
      if (exp->concrete_type() == Expression::NULL_VAL) {
        std::stringstream msg;
        msg << argname << ": null is not a string for `" << function_name(sig) << "'";
        error(msg.str(), exp->pstate(), traces);
      }
      if (String_Constant* str = Cast<String_Constant>(exp)) {
        str->quote_mark(0);
      }
      std::string exp_src = exp->to_string(ctx.c_options);
      SelectorListObj sel_list = Parser::parse_selector(exp_src.c_str(), ctx, traces, exp->pstate(), pstate.src, /*allow_parent=*/false);
      if (sel_list->length() == 0) return {};
      return sel_list->first()->first();
    }


  }

}
