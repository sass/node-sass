#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "prefix_primitives.h"

char *prefix_is_char(char *src, char pre) {
  return pre == *src ? src+1 : NULL;
}

char *prefix_is_chars(char *src, char *pre) {
  while (*pre && *src == *pre) src++, pre++;
  return *pre ? NULL : src;
}

char *prefix_is_one_of(char *src, char *class) {
  while(*class && *src != *class) class++;
  return *class ? src+1 : NULL;
}

char *prefix_is_some_of(char *src, char *class) {
  char *p = src;
  while(prefix_is_one_of(p, class)) p++;
  return p == src ? NULL : p;
}

char *prefix_is_delimited_by(char *src, char *beg, char *end, int esc) {
  src = prefix_is_chars(src, beg);
  if (!src) return NULL;
  char *stop;
  while (1) {
    if (!*src) return NULL;
    stop = prefix_is_chars(src, end);
    if (stop && (!esc || *(src-1) != '\\')) return stop;
    src = stop ? stop : src+1;
  }
}

char *_prefix_alternatives(char *src, ...) {
  va_list ap;
  va_start(ap, src);
  prefix_matcher m = va_arg(ap, prefix_matcher);
  char *p = NULL;
  while (m && !(p = (*m)(src))) m = va_arg(ap, prefix_matcher);
  va_end(ap);
  return p;
}

char *_prefix_sequence(char *src, ...) {
  va_list ap;
  va_start(ap, src);
  prefix_matcher m;
  while ((m = va_arg(ap, prefix_matcher)) && (src = (*m)(src))) ;
  return src;
}

char *prefix_optional(char *src, prefix_matcher m) {
  char *p = m(src);
  return p ? p : src;
}

char *prefix_zero_plus(char *src, prefix_matcher m) {
  char *p = m(src);
  while(p) src = p, p = m(src);
  return src;
}

char *prefix_one_plus(char *src, prefix_matcher m) {
  char *p = m(src);
  if (!p) return NULL;
  while(p) src = p, p = m(src);
  return src;
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

DEFINE_TO_EOL_MATCHER(shell_comment, "#");
DEFINE_TO_EOL_MATCHER(c_line_comment, "//");
DEFINE_DELIMITED_MATCHER(c_block_comment, "/*", "*/", 0);
DEFINE_DELIMITED_MATCHER(double_quoted_string, "\"", "\"", 1);
DEFINE_DELIMITED_MATCHER(single_quoted_string, "\'", "\'", 1);
DEFINE_DELIMITED_MATCHER(interpolant, "#{", "}", 0);

DEFINE_CHAR_MATCHER (lparen,      '(');
DEFINE_CHAR_MATCHER (rparen,      ')');
DEFINE_CHAR_MATCHER (lbrack,      '[');
DEFINE_CHAR_MATCHER (rbrack,      ']');
DEFINE_CHAR_MATCHER (lbrace,      '{');
DEFINE_CHAR_MATCHER (rbrace,      '}');

DEFINE_CHAR_MATCHER (underscore,  '_');
DEFINE_CHAR_MATCHER (hyphen,      '-');
DEFINE_CHAR_MATCHER (semicolon,   ';');
DEFINE_CHAR_MATCHER (colon,       ':');
DEFINE_CHAR_MATCHER (period,      '.');
DEFINE_CHAR_MATCHER (question,    '?');
DEFINE_CHAR_MATCHER (exclamation, '!');
DEFINE_CHAR_MATCHER (tilde,       '~');
DEFINE_CHAR_MATCHER (backquote,   '`');
DEFINE_CHAR_MATCHER (quote,       '\"');
DEFINE_CHAR_MATCHER (apostrophe,  '\'');
DEFINE_CHAR_MATCHER (ampersand,   '&');
DEFINE_CHAR_MATCHER (caret,       '^');
DEFINE_CHAR_MATCHER (pipe,        '|');
DEFINE_CHAR_MATCHER (slash,       '/');
DEFINE_CHAR_MATCHER (backslash,   '\\');
DEFINE_CHAR_MATCHER (asterisk,    '*');
DEFINE_CHAR_MATCHER (pound,       '#');
DEFINE_CHAR_MATCHER (hash,        '#');

DEFINE_CHAR_MATCHER (plus,        '+');
DEFINE_CHAR_MATCHER (minus,       '-');
DEFINE_CHAR_MATCHER (times,       '*');
DEFINE_CHAR_MATCHER (divide,      '/');

DEFINE_CHAR_MATCHER (percent,     '%');
DEFINE_CHAR_MATCHER (dollar,      '$');

DEFINE_CHAR_MATCHER (gt,          '>');
DEFINE_CHARS_MATCHER(gte,         ">=");
DEFINE_CHAR_MATCHER (lt,          '<');
DEFINE_CHARS_MATCHER(lte,         "<=");
DEFINE_CHAR_MATCHER (eq,          '=');
DEFINE_CHAR_MATCHER (assign,      '=');
DEFINE_CHARS_MATCHER(equal,       "==");

static DEFINE_ALTERNATIVES_MATCHER(identifier_initial, prefix_is_alphas, prefix_is_underscore);
static DEFINE_ALTERNATIVES_MATCHER(identifier_trailer, prefix_is_alnums, prefix_is_underscore);
DEFINE_FIRST_REST_MATCHER(identifier, prefix_is_identifier_initial, prefix_is_identifier_trailer);

