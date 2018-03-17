#ifndef SASS_FUNCTIONS_H
#define SASS_FUNCTIONS_H

#include "listize.hpp"
#include "position.hpp"
#include "environment.hpp"
#include "ast_fwd_decl.hpp"
#include "sass/functions.h"
#include "fn_utils.hpp"

namespace Sass {
  struct Backtrace;
  typedef const char* Signature;
  typedef Expression_Ptr (*Native_Function)(Env&, Env&, Context&, Signature, ParserState, Backtraces, std::vector<Selector_List_Obj>);

  std::string function_name(Signature);

  namespace Functions {

    extern Signature type_of_sig;
    extern Signature variable_exists_sig;
    extern Signature global_variable_exists_sig;
    extern Signature function_exists_sig;
    extern Signature mixin_exists_sig;
    extern Signature feature_exists_sig;
    extern Signature call_sig;
    extern Signature not_sig;
    extern Signature if_sig;
    extern Signature map_get_sig;
    extern Signature map_merge_sig;
    extern Signature map_remove_sig;
    extern Signature map_keys_sig;
    extern Signature map_values_sig;
    extern Signature map_has_key_sig;
    extern Signature content_exists_sig;
    extern Signature get_function_sig;

    BUILT_IN(type_of);
    BUILT_IN(variable_exists);
    BUILT_IN(global_variable_exists);
    BUILT_IN(function_exists);
    BUILT_IN(mixin_exists);
    BUILT_IN(feature_exists);
    BUILT_IN(call);
    BUILT_IN(sass_not);
    BUILT_IN(sass_if);
    BUILT_IN(map_get);
    BUILT_IN(map_merge);
    BUILT_IN(map_remove);
    BUILT_IN(map_keys);
    BUILT_IN(map_values);
    BUILT_IN(map_has_key);
    BUILT_IN(content_exists);
    BUILT_IN(get_function);
  }
}

#endif
