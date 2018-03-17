#ifndef SASS_FN_UTILS_H
#define SASS_FN_UTILS_H

#include "backtrace.hpp"
#include "environment.hpp"
#include "ast_fwd_decl.hpp"

namespace Sass {

  #define BUILT_IN(name) Expression_Ptr \
    name(Env& env, Env& d_env, Context& ctx, Signature sig, ParserState pstate, Backtraces traces, std::vector<Selector_List_Obj> selector_stack)

  #define ARG(argname, argtype) get_arg<argtype>(argname, env, sig, pstate, traces)
  #define ARGM(argname, argtype, ctx) get_arg_m(argname, env, sig, pstate, traces)

  // special function for weird hsla percent (10px == 10% == 10 != 0.1)
  #define ARGVAL(argname) get_arg_val(argname, env, sig, pstate, traces) // double

  // macros for common ranges (u mean unsigned or upper, r for full range)
  #define DARG_U_FACT(argname) get_arg_r(argname, env, sig, pstate, traces, - 0.0, 1.0) // double
  #define DARG_R_FACT(argname) get_arg_r(argname, env, sig, pstate, traces, - 1.0, 1.0) // double
  #define DARG_U_BYTE(argname) get_arg_r(argname, env, sig, pstate, traces, - 0.0, 255.0) // double
  #define DARG_R_BYTE(argname) get_arg_r(argname, env, sig, pstate, traces, - 255.0, 255.0) // double
  #define DARG_U_PRCT(argname) get_arg_r(argname, env, sig, pstate, traces, - 0.0, 100.0) // double
  #define DARG_R_PRCT(argname) get_arg_r(argname, env, sig, pstate, traces, - 100.0, 100.0) // double

  // macros for color related inputs (rbg and alpha/opacity values)
  #define COLOR_NUM(argname) color_num(argname, env, sig, pstate, traces) // double
  #define ALPHA_NUM(argname) alpha_num(argname, env, sig, pstate, traces) // double

  #define ARGSEL(argname) get_arg_sel(argname, env, sig, pstate, traces, ctx)
  #define ARGSELS(argname) get_arg_sels(argname, env, sig, pstate, traces, ctx)

  typedef const char* Signature;

  typedef Expression_Ptr (*Native_Function)(Env&, Env&, Context&, Signature, ParserState, Backtraces, SelectorStack);

  Definition_Ptr make_native_function(Signature, Native_Function, Context& ctx);
  Definition_Ptr make_c_function(Sass_Function_Entry c_func, Context& ctx);

  namespace Functions {

    template <typename T>
    T* get_arg(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces)
    {
      T* val = Cast<T>(env[argname]);
      if (!val) {
        std::string msg("argument `");
        msg += argname;
        msg += "` of `";
        msg += sig;
        msg += "` must be a ";
        msg += T::type_name();
        error(msg, pstate, traces);
      }
      return val;
    }

    Map_Ptr get_arg_m(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces); // maps only
    Number_Ptr get_arg_n(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces); // numbers only
    double alpha_num(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces); // colors only
    double color_num(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces); // colors only
    double get_arg_r(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, double lo, double hi); // colors only
    double get_arg_val(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces); // shared
    Selector_List_Obj get_arg_sels(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, Context& ctx); // selectors only
    Compound_Selector_Obj get_arg_sel(const std::string& argname, Env& env, Signature sig, ParserState pstate, Backtraces traces, Context& ctx); // selectors only

  }

}

#endif
