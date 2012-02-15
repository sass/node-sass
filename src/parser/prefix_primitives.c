#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "prefix_primitives.h"

int prefix_is_exactly(char *src, char* pre) {
  int p;
  for (p = 0; pre[p] && src[p] == pre[p]; p++) ;
  return pre[p] ? 0 : p;
}

int prefix_is_one_of(char *src, char *class) {
  int i;
  for (i = 0; class[i] && src[0] != class[i]; i++) ;
  return class[i] ? 1 : 0;
}

int prefix_is_some_of(char *src, char *class) {
  int p;
  for (p = 0; prefix_is_one_of(src+p, class); p++) ;
  return p;
}

int prefix_is_delimited_by(char *src, char *beg, char *end, int esc) {
  int p, len  = prefix_is_exactly(src, beg);
  if (!len) return 0;
  p = len;
  while (1) {
    if (src[p] == '\0') return 0;
    len = prefix_is_exactly(src+p, end);
    if (len && (!esc || src[p-1] != '\\')) return p + len;
    p += len ? len : 1;
  }
}

int _prefix_alternatives(char *src, ...) {
  int p = 0;
  va_list ap;
  va_start(ap, src);
  prefix_matcher m = va_arg(ap, prefix_matcher);  
  while (m && !(p = (*m)(src))) m = va_arg(ap, prefix_matcher);
  va_end(ap);
  return p;
}

int _prefix_sequence(char *src, ...) {
  int p = 0, p_sum = 0;
  va_list ap;
  va_start(ap, src);
  prefix_matcher m = va_arg(ap, prefix_matcher);
  while (m && (p = (*m)(src))) p_sum += p, m = va_arg(ap, prefix_matcher);
  va_end(ap);
  return p ? p_sum : 0;
}

int prefix_optional(char *src, prefix_matcher m) {
  int p = m(src);
  return p ? p : -1;
}

DEFINE_SINGLE_CTYPE_MATCHER(space);
DEFINE_SINGLE_CTYPE_MATCHER(alpha);
DEFINE_SINGLE_CTYPE_MATCHER(digit);
DEFINE_SINGLE_CTYPE_MATCHER(xdigit);
DEFINE_SINGLE_CTYPE_MATCHER(alnum);
DEFINE_SINGLE_CTYPE_MATCHER(punct);
DEFINE_CTYPE_SEQUENCE_MATCHER(space);
DEFINE_CTYPE_SEQUENCE_MATCHER(alpha);
DEFINE_CTYPE_SEQUENCE_MATCHER(digit);
DEFINE_CTYPE_SEQUENCE_MATCHER(xdigit);
DEFINE_CTYPE_SEQUENCE_MATCHER(alnum);
DEFINE_CTYPE_SEQUENCE_MATCHER(punct);
DEFINE_TO_EOL_MATCHER(line_comment, "//");
DEFINE_DELIMITED_MATCHER(block_comment, "/*", "*/", 0);
DEFINE_DELIMITED_MATCHER(double_quoted_string, "\"", "\"", 1);
DEFINE_DELIMITED_MATCHER(single_quoted_string, "'", "'", 1);
DEFINE_DELIMITED_MATCHER(interpolant, "#{", "}", 0);
DEFINE_ALTERNATIVES_MATCHER(string, prefix_is_double_quoted_string,
                                    prefix_is_single_quoted_string);

DEFINE_EXACT_MATCHER(lparen,      "(");
DEFINE_EXACT_MATCHER(rparen,      ")");
DEFINE_EXACT_MATCHER(lbrack,      "[");
DEFINE_EXACT_MATCHER(rbrack,      "]");
DEFINE_EXACT_MATCHER(lbrace,      "{");
DEFINE_EXACT_MATCHER(rbrace,      "}");

DEFINE_EXACT_MATCHER(underscore,  "_");
DEFINE_EXACT_MATCHER(hyphen,      "-");
DEFINE_EXACT_MATCHER(semicolon,   ";");
DEFINE_EXACT_MATCHER(colon,       ":");
DEFINE_EXACT_MATCHER(period,      ".");
DEFINE_EXACT_MATCHER(question,    "?");
DEFINE_EXACT_MATCHER(exclamation, "!");
DEFINE_EXACT_MATCHER(tilde,       "~");
DEFINE_EXACT_MATCHER(backquote,   "`");
DEFINE_EXACT_MATCHER(quote,       "\"");
DEFINE_EXACT_MATCHER(apostrophe,  "'");
DEFINE_EXACT_MATCHER(ampersand,   "&");
DEFINE_EXACT_MATCHER(caret,       "^");
DEFINE_EXACT_MATCHER(pipe,        "|");
DEFINE_EXACT_MATCHER(slash,       "/");
DEFINE_EXACT_MATCHER(backslash,   "\\");
DEFINE_EXACT_MATCHER(asterisk,    "*");
DEFINE_EXACT_MATCHER(pound,       "#");
DEFINE_EXACT_MATCHER(hash,        "#");

DEFINE_EXACT_MATCHER(plus,        "+");
DEFINE_EXACT_MATCHER(minus,       "-");
DEFINE_EXACT_MATCHER(times,       "*");
DEFINE_EXACT_MATCHER(divide,      "/");

DEFINE_EXACT_MATCHER(percent,     "%");
DEFINE_EXACT_MATCHER(dollar,      "$");

DEFINE_EXACT_MATCHER(gt,          ">");
DEFINE_EXACT_MATCHER(gte,         ">=");
DEFINE_EXACT_MATCHER(lt,          "<");
DEFINE_EXACT_MATCHER(lte,         "<=");
DEFINE_EXACT_MATCHER(eq,          "=");
DEFINE_EXACT_MATCHER(assign,      "=");
DEFINE_EXACT_MATCHER(equal,       "==");

DEFINE_ALTERNATIVES_MATCHER(identifier_initial, prefix_is_alphas, prefix_is_underscore);
DEFINE_ALTERNATIVES_MATCHER(identifier_trailing, prefix_is_alnums, prefix_is_underscore);
DEFINE_FIRST_REST_MATCHER(identifier, prefix_is_identifier_initial, prefix_is_identifier_trailing);

DEFINE_SEQUENCE_MATCHER(hyphen_and_alpha, prefix_is_hyphen, prefix_is_alpha);
DEFINE_ALTERNATIVES_MATCHER(word_initial, prefix_is_identifier, prefix_is_hyphen_and_alpha);
DEFINE_ALTERNATIVES_MATCHER(word_trailing, prefix_is_alnums, prefix_is_hyphen, prefix_is_underscore);
DEFINE_FIRST_REST_MATCHER(word, prefix_is_word_initial, prefix_is_word_trailing);
