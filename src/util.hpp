#ifndef SASS_UTIL_H
#define SASS_UTIL_H

#include <vector>
#include <string>
#include <assert.h>
#include "sass/base.h"
#include "ast_fwd_decl.hpp"

#define SASS_ASSERT(cond, msg) assert(cond && msg)

namespace Sass {

  double round(double val);
  char* sass_strdup(const char* str);
  double sass_atof(const char* str);
  const char* safe_str(const char *, const char* = "");
  void free_string_array(char **);
  char **copy_strings(const std::vector<std::string>&, char ***, int = 0);
  std::string string_escape(const std::string& str);
  std::string string_unescape(const std::string& str);
  std::string string_eval_escapes(const std::string& str);
  std::string read_css_string(const std::string& str);
  std::string evacuate_quotes(const std::string& str);
  std::string evacuate_escapes(const std::string& str);
  std::string string_to_output(const std::string& str);
  std::string comment_to_string(const std::string& text);
  std::string normalize_wspace(const std::string& str);

  std::string quote(const std::string&, char q = 0, bool keep_linefeed_whitespace = false);
  std::string unquote(const std::string&, char* q = 0, bool keep_utf8_sequences = false);
  char detect_best_quotemark(const char* s, char qm = '"');

  bool is_hex_doublet(double n);
  bool is_color_doublet(double r, double g, double b);

  bool peek_linefeed(const char* start);

  namespace Util {

    std::string rtrim(const std::string& str);

    std::string normalize_underscores(const std::string& str);
    std::string normalize_decimals(const std::string& str);
    std::string normalize_sixtuplet(const std::string& col);

    std::string vecJoin(const std::vector<std::string>& vec, const std::string& sep);
    bool containsAnyPrintableStatements(Block* b);

    bool isPrintable(Ruleset* r, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(Supports_Block* r, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(Media_Block* r, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(Block* b, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(String_Constant* s, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(String_Quoted* s, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isPrintable(Declaration* d, Sass_Output_Style style = SASS_STYLE_NESTED);
    bool isAscii(const char chr);

  }
}
#endif
