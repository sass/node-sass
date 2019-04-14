#ifndef SASS_UTIL_STRING_H
#define SASS_UTIL_STRING_H

#include <string>

namespace Sass {
namespace Util {

std::string rtrim(const std::string& str);

std::string normalize_newlines(const std::string& str);
std::string normalize_underscores(const std::string& str);
std::string normalize_decimals(const std::string& str);
char opening_bracket_for(char closing_bracket);
char closing_bracket_for(char opening_bracket);

}  // namespace Sass
}  // namespace Util
#endif  // SASS_UTIL_STRING_H
