#ifndef SASS_UTIL_STRING_H
#define SASS_UTIL_STRING_H

#include <string>

namespace Sass {
  namespace Util {

    // ##########################################################################
    // Special case insensitive string matcher. We can optimize
    // the more general compare case quite a bit by requiring
    // consumers to obey some rules (lowercase and no space).
    // - `literal` must only contain lower case ascii characters
    // there is one edge case where this could give false positives
    // test could contain a (non-ascii) char exactly 32 below literal
    // ##########################################################################
    bool equalsLiteral(const char* lit, const std::string& test);

    // ###########################################################################
    // Returns [name] without a vendor prefix.
    // If [name] has no vendor prefix, it's returned as-is.
    // ###########################################################################
    std::string unvendor(const std::string& name);

    std::string rtrim(const std::string& str);
    std::string normalize_newlines(const std::string& str);
    std::string normalize_underscores(const std::string& str);
    std::string normalize_decimals(const std::string& str);
    char opening_bracket_for(char closing_bracket);
    char closing_bracket_for(char opening_bracket);

  }
  // namespace Util
}  
// namespace Sass

#endif  // SASS_UTIL_STRING_H
