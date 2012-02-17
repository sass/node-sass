#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
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

char *prefix_epsilon(char *src) {
  return src;
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

SINGLE_CTYPE_MATCHER(space);
SINGLE_CTYPE_MATCHER(alpha);
SINGLE_CTYPE_MATCHER(digit);
SINGLE_CTYPE_MATCHER(xdigit);
SINGLE_CTYPE_MATCHER(alnum);
SINGLE_CTYPE_MATCHER(punct);

CTYPE_SEQUENCE_MATCHER(space);
CTYPE_SEQUENCE_MATCHER(alpha);
CTYPE_SEQUENCE_MATCHER(digit);
CTYPE_SEQUENCE_MATCHER(xdigit);
CTYPE_SEQUENCE_MATCHER(alnum);
CTYPE_SEQUENCE_MATCHER(punct);

TO_EOL_MATCHER(shell_comment, "#");
TO_EOL_MATCHER(c_line_comment, "//");
DELIMITED_MATCHER(c_block_comment, "/*", "*/", 0);
DELIMITED_MATCHER(double_quoted_string, "\"", "\"", 1);
DELIMITED_MATCHER(single_quoted_string, "\'", "\'", 1);
DELIMITED_MATCHER(interpolant, "#{", "}", 0);

CHAR_MATCHER (lparen,      '(');
CHAR_MATCHER (rparen,      ')');
CHAR_MATCHER (lbrack,      '[');
CHAR_MATCHER (rbrack,      ']');
CHAR_MATCHER (lbrace,      '{');
CHAR_MATCHER (rbrace,      '}');

CHAR_MATCHER (underscore,  '_');
CHAR_MATCHER (hyphen,      '-');
CHAR_MATCHER (semicolon,   ';');
CHAR_MATCHER (colon,       ':');
CHAR_MATCHER (period,      '.');
CHAR_MATCHER (question,    '?');
CHAR_MATCHER (exclamation, '!');
CHAR_MATCHER (tilde,       '~');
CHAR_MATCHER (backquote,   '`');
CHAR_MATCHER (quote,       '\"');
CHAR_MATCHER (apostrophe,  '\'');
CHAR_MATCHER (ampersand,   '&');
CHAR_MATCHER (caret,       '^');
CHAR_MATCHER (pipe,        '|');
CHAR_MATCHER (slash,       '/');
CHAR_MATCHER (backslash,   '\\');
CHAR_MATCHER (asterisk,    '*');
CHAR_MATCHER (pound,       '#');
CHAR_MATCHER (hash,        '#');

CHAR_MATCHER (plus,        '+');
CHAR_MATCHER (minus,       '-');
CHAR_MATCHER (times,       '*');
CHAR_MATCHER (divide,      '/');

CHAR_MATCHER (percent,     '%');
CHAR_MATCHER (dollar,      '$');

CHAR_MATCHER (gt,          '>');
CHARS_MATCHER(gte,         ">=");
CHAR_MATCHER (lt,          '<');
CHARS_MATCHER(lte,         "<=");
CHAR_MATCHER (eq,          '=');
CHAR_MATCHER (assign,      '=');
CHARS_MATCHER(equal,       "==");

static ALTERNATIVES_MATCHER(identifier_initial, prefix_is_alphas, prefix_is_underscore);
static ALTERNATIVES_MATCHER(identifier_trailer, prefix_is_alnums, prefix_is_underscore);
FIRST_REST_MATCHER(identifier, prefix_is_identifier_initial, prefix_is_identifier_trailer);

static ALTERNATIVES_MATCHER(optional_sign, prefix_is_plus, prefix_is_minus, prefix_epsilon);
SEQUENCE_MATCHER(integer, prefix_is_optional_sign, prefix_is_digits);