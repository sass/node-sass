#ifndef SASS_C2AST_H
#define SASS_C2AST_H

#include "position.hpp"
#include "backtrace.hpp"
#include "ast_fwd_decl.hpp"

namespace Sass {

  Value_Ptr c2ast(union Sass_Value* v, Backtraces traces, ParserState pstate);

}

#endif
