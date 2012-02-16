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
DEFINE_TO_EOL_MATCHER(line_comment, "//");
DEFINE_DELIMITED_MATCHER(block_comment, "/*", "*/", 0);
DEFINE_DELIMITED_MATCHER(double_quoted_string, "\"", "\"", 1);
DEFINE_DELIMITED_MATCHER(single_quoted_string, "\'", "\'", 1);
DEFINE_DELIMITED_MATCHER(interpolant, "#{", "}", 0);

// int main() {
//   char *p = prefix_sequence("hello", prefix_is_alphas);
//   if (!*p) putchar('0');
//   return 0;
// }