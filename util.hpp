#ifndef SASS_UTIL_H
#define SASS_UTIL_H

#include "ast.hpp"
#include <vector>
#include <string>

namespace Sass {
  using namespace std;

  string string_escape(const string& str);
  string string_unescape(const string& str);
  string evacuate_quotes(const string& str);
  string evacuate_escapes(const string& str);
  string string_to_output(const string& str);

  namespace Util {

    string normalize_underscores(const string& str);
    string normalize_decimals(const string& str);
    string normalize_sixtuplet(const string& col);

    string vecJoin(const vector<string>& vec, const string& sep);
    bool containsAnyPrintableStatements(Block* b);

    bool isPrintable(Ruleset* r);
    bool isPrintable(Feature_Block* r);
    bool isPrintable(Media_Block* r);
    bool isPrintable(Block* b);
    bool isAscii(int ch);

  }
}
#endif
