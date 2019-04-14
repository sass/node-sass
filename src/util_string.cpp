#include "util_string.hpp"

#include <algorithm>

namespace Sass {
namespace Util {

std::string rtrim(const std::string &str) {
  std::string trimmed = str;
  size_t pos_ws = trimmed.find_last_not_of(" \t\n\v\f\r");
  if (pos_ws != std::string::npos) {
    trimmed.erase(pos_ws + 1);
  } else {
    trimmed.clear();
  }
  return trimmed;
}

std::string normalize_newlines(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  std::size_t pos = 0;
  while (true) {
    const std::size_t newline = str.find_first_of("\n\f\r", pos);
    if (newline == std::string::npos) break;
    result.append(str, pos, newline - pos);
    result += '\n';
    if (str[newline] == '\r' && str[newline + 1] == '\n') {
      pos = newline + 2;
    } else {
      pos = newline + 1;
    }
  }
  result.append(str, pos, std::string::npos);
  return result;
}

std::string normalize_underscores(const std::string& str) {
  std::string normalized = str;
  std::replace(normalized.begin(), normalized.end(), '_', '-');
  return normalized;
}

std::string normalize_decimals(const std::string& str) {
  std::string normalized;
  if (!str.empty() && str[0] == '.') {
    normalized.reserve(str.size() + 1);
    normalized += '0';
    normalized += str;
  } else {
    normalized = str;
  }
  return normalized;
}

char opening_bracket_for(char closing_bracket) {
  switch (closing_bracket) {
    case ')': return '(';
    case ']': return '[';
    case '}': return '{';
    default: return '\0';
  }
}

char closing_bracket_for(char opening_bracket) {
  switch (opening_bracket) {
    case '(': return ')';
    case '[': return ']';
    case '{': return '}';
    default: return '\0';
  }
}

}  // namespace Sass
}  // namespace Util
