#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "prefix.h"

char *sass_prefix_is_char(char *src, char pre) {
  return pre == *src ? src+1 : NULL;
}

char *sass_prefix_is_chars(char *src, char *pre) {
  while (*pre && *src == *pre) src++, pre++;
  return *pre ? NULL : src;
}

char *sass_prefix_is_one_of(char *src, char *class) {
  while(*class && *src != *class) class++;
  return *class ? src+1 : NULL;
}

char *sass_prefix_is_some_of(char *src, char *class) {
  char *p = src;
  while(sass_prefix_is_one_of(p, class)) p++;
  return p == src ? NULL : p;
}

char *sass_prefix_is_delimited_by(char *src, char *beg, char *end, int esc) {
  src = sass_prefix_is_chars(src, beg);
  if (!src) return NULL;
  char *stop;
  while (1) {
    if (!*src) return NULL;
    stop = sass_prefix_is_chars(src, end);
    if (stop && (!esc || *(src-1) != '\\')) return stop;
    src = stop ? stop : src+1;
  }
}

char *sass_prefix_epsilon(char *src) {
  return src;
}

char *sass_prefix_not(char *src, sass_prefix_matcher m) {
  return m(src) ? NULL : src;
}

char *_sass_prefix_alternatives(char *src, ...) {
  va_list ap;
  va_start(ap, src);
  sass_prefix_matcher m = va_arg(ap, sass_prefix_matcher);
  char *p = NULL;
  while (m && !(p = (*m)(src))) m = va_arg(ap, sass_prefix_matcher);
  va_end(ap);
  return p;
}

char *_sass_prefix_sequence(char *src, ...) {
  va_list ap;
  va_start(ap, src);
  sass_prefix_matcher m;
  while ((m = va_arg(ap, sass_prefix_matcher)) && (src = (*m)(src))) ;
  return src;
}

char *sass_prefix_optional(char *src, sass_prefix_matcher m) {
  char *p = m(src);
  return p ? p : src;
}

char *sass_prefix_zero_plus(char *src, sass_prefix_matcher m) {
  char *p = m(src);
  while(p) src = p, p = m(src);
  return src;
}

char *sass_prefix_one_plus(char *src, sass_prefix_matcher m) {
  char *p = m(src);
  if (!p) return NULL;
  while(p) src = p, p = m(src);
  return src;
}

char *sass_prefix_find_first(char *src, sass_prefix_matcher m) {
  while (*src && !m(src)) src++;
  return *src ? src : NULL;
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

TO_EOL_MATCHER(line_comment, "//");
DELIMITED_MATCHER(block_comment, "/*", "*/", 0);
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
CHAR_MATCHER (dot,         '.');
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
CHAR_MATCHER (star,        '*');
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

CHAR_MATCHER (exactmatch,     '=');
CHARS_MATCHER(includes,       "~=");
CHARS_MATCHER(dashmatch,      "|=");
CHARS_MATCHER(prefixmatch,    "^=");
CHARS_MATCHER(suffixmatch,    "$=");
CHARS_MATCHER(substringmatch, "*=");

static OPTIONAL_MATCHER(optional_hyphen, sass_prefix_is_hyphen);
static ALTERNATIVES_MATCHER(nmchar, sass_prefix_is_alnums, sass_prefix_is_underscore, sass_prefix_is_hyphen);
static ALTERNATIVES_MATCHER(nmstart, sass_prefix_is_alphas, sass_prefix_is_underscore);
static SEQUENCE_MATCHER(identstart, sass_prefix_is_optional_hyphen, sass_prefix_is_nmstart);
ONE_PLUS_MATCHER(name, sass_prefix_is_nmchar);
FIRST_REST_MATCHER(identifier, sass_prefix_is_identstart, sass_prefix_is_nmchar);

SEQUENCE_MATCHER(variable, sass_prefix_is_dollar, sass_prefix_is_identifier);

static OPTIONAL_MATCHER(optional_digits, sass_prefix_is_digits);
static SEQUENCE_MATCHER(realnum, sass_prefix_is_optional_digits, sass_prefix_is_dot, sass_prefix_is_digits);
ALTERNATIVES_MATCHER(number, sass_prefix_is_digits, sass_prefix_is_realnum);
ALTERNATIVES_MATCHER(string, sass_prefix_is_double_quoted_string, sass_prefix_is_single_quoted_string);

SEQUENCE_MATCHER(idname, sass_prefix_is_hash, sass_prefix_is_name);
SEQUENCE_MATCHER(classname, sass_prefix_is_dot, sass_prefix_is_identifier);
SEQUENCE_MATCHER(function, sass_prefix_is_identifier, sass_prefix_is_lparen);

static OPTIONAL_MATCHER(optional_spaces, sass_prefix_is_spaces);
SEQUENCE_MATCHER(adjacent_to, sass_prefix_is_optional_spaces, sass_prefix_is_plus);
SEQUENCE_MATCHER(precedes, sass_prefix_is_optional_spaces, sass_prefix_is_tilde);
SEQUENCE_MATCHER(parent_of, sass_prefix_is_optional_spaces, sass_prefix_is_gt);
ALIAS_MATCHERS(sass_prefix_is_spaces, ancestor_of);