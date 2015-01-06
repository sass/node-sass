#ifndef SASS_UTIL_H
#define SASS_UTIL_H

#include "ast.hpp"

#include <string>
namespace Sass {
  namespace Util {
    using namespace std;

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
