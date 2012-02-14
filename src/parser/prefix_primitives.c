#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "prefix_primitives.h"

int prefix_is_exactly(char *src, char* pre) {
  int p = 0;
  while (1) {
    if (pre[p] == '\0') return p;
    if (pre[p] != src[p]) return 0;
    p++;
  }
}

int prefix_is_one_of(char *src, char *class) {
  int i;
  for (i = 0; class[i]; i++) if (class[i] == src[0]) return 1;
  return 0;
}

int prefix_is_some_of(char *src, char *class) {
  int p;
  for (p = 0; prefix_is_one_of(src+p, class); p++) ;
  return p;
}

int prefix_is_delimited_by(char *src, char *beg, char *end, int esc) {
  int p = 0;
  int len = prefix_is_exactly(src, beg);
  if (!len) return 0;
  p += len;
  while (1) {
    if (src[p] == '\0') return 0;
    len = prefix_is_exactly(src+p, end);
    if (len && (!esc || src[p-1] != '\\')) return p + len;
    p += len ? len : 1;
  }
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

int prefix_is_string(char *src) {
  int len = prefix_is_double_quoted_string(src);
  return len ? len : prefix_is_single_quoted_string(src);
}

DEFINE_EXACT_MATCHER(lparen, "(");
DEFINE_EXACT_MATCHER(rparen, ")");
DEFINE_EXACT_MATCHER(lbrack, "[");
DEFINE_EXACT_MATCHER(rbrack, "]");
DEFINE_EXACT_MATCHER(lbrace, "{");
DEFINE_EXACT_MATCHER(rbrace, "}");
DEFINE_EXACT_MATCHER(asterisk, "*");

/* not sure I'm gonna' need these
DEFINE_EXACT_MATCHER(exclamation, "!");
DEFINE_EXACT_MATCHER(pound, "#");
DEFINE_EXACT_MATCHER(hash, "#");
DEFINE_EXACT_MATCHER(dollar, "$");
DEFINE_EXACT_MATCHER(percent, "%");
DEFINE_EXACT_MATCHER(ampersand, "&");
DEFINE_EXACT_MATCHER(lparen, "(");
DEFINE_EXACT_MATCHER(rparen, ")");
DEFINE_EXACT_MATCHER(times, "*");
DEFINE_EXACT_MATCHER(comma, ",");
DEFINE_EXACT_MATCHER(hyphen, "-");
DEFINE_EXACT_MATCHER(minus, "-");
DEFINE_EXACT_MATCHER(period, ".");
DEFINE_EXACT_MATCHER(dot, ".");
DEFINE_EXACT_MATCHER(slash, "/");
DEFINE_EXACT_MATCHER(divide, "/");
DEFINE_EXACT_MATCHER(colon, ":");
DEFINE_EXACT_MATCHER(semicolon, ";");
DEFINE_EXACT_MATCHER(lt, "<");
DEFINE_EXACT_MATCHER(lte, "<=");
DEFINE_EXACT_MATCHER(gt, ">");
DEFINE_EXACT_MATCHER(gte, ">=");
*/