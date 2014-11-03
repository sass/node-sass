#ifndef SASS_UTIL
#define SASS_UTIL

#ifndef SASS_AST
#include "ast.hpp"
#endif

#include <string>
namespace Sass {
  namespace Util {

    std::string normalize_underscores(const std::string& str);

    bool containsAnyPrintableStatements(Block* b);

    bool isPrintable(Ruleset* r);
    bool isPrintable(Feature_Block* r);
    bool isPrintable(Media_Block* r);
    bool isPrintable(Block* b);

  }
}
#endif
