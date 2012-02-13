#include <ctype.h>
#include <string.h>
#include "prefix_primitives.h"

int prefix_is_exactly(char *src, char* pre) {
  int p = 0;
  while (1) {
    if (pre[p] == '\0') return p;
    if (pre[p] != src[p]) return 0;
    p++;
  }
}

int prefix_is_delimited_by(char *src, char *beg, char *end, int esc) {
  int p = 0;
  int len = prefix_is_exactly(src, beg);
  if (!len) return 0;
  p += len;
  while (1) {
    if (src[p] == '\0') return 0;
    len = prefix_is_exactly(src+p, end);
    if (len && esc && src[p-1] != '\\') return p + len;
    p += len ? len : 1;
  }
}

DEFINE_CTYPE_SEQUENCE_MATCHER(space);
DEFINE_CTYPE_SEQUENCE_MATCHER(alpha);
DEFINE_CTYPE_SEQUENCE_MATCHER(digit);
DEFINE_CTYPE_SEQUENCE_MATCHER(xdigit);
DEFINE_CTYPE_SEQUENCE_MATCHER(alnum);
DEFINE_TO_EOL_MATCHER(line_comment, "//");
DEFINE_DELIMITED_MATCHER(block_comment, "/*", "*/", 0);
DEFINE_DELIMITED_MATCHER(dqstring, "\"", "\"", 1);
DEFINE_DELIMITED_MATCHER(sqstring, "'", "'", 1);
DEFINE_DELIMITED_MATCHER(interpolant, "#{", "}", 0);

int prefix_is_string_constant(char *src) {
  int len = prefix_is_dqstring(src);
  len = len ? len : prefix_is_sqstring(src);
  return len;
}
