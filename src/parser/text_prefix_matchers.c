#include <ctype.h>
#include <string.h>

#define text_has_ctype_prefix(src, type) (is ## type((src)[0]) ? 1 : 0)

#define DEF_SINGLE_CTYPE_PREFIX_MATCHER(type) \
size_t text_starts_with_one_ ## type(char *src) \
  return is ## type(src[0]) ? 1 : 0; \
}

#define DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(type) \
size_t text_starts_with_ ## type ## s(char *src) { \
  int p = 0; \
  while (is ## type(src[p])) p++; \
  return p; \
}


size_t text_has_exact_prefix(char *src, char* pre) {
  int p = 0;
  while (1) {
    if (pre[p] == '\0') return p;
    if (pre[p] != src[p]) return 0;
    p++;
  }
}

size_t text_has_delimited_prefix(char *src, char *beg, char *end, int esc) {
  int p = 0;
  size_t len = text_has_exact_prefix(src, beg);
  if (!len) return 0;
  p += len;
  while (1) {
    if (src[p] == '\0') return 0;
    len = text_has_exact_prefix(src+p, end);
    if (len && esc && src[p-1] != '\\') return p + len;
    p += len || 1;
  }
}

DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(space);  /* size_t text_starts_with_spaces(char *)  */
DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(alpha);  /* size_t text_starts_with_alphas(char *)  */
DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(digit);  /* size_t text_starts_with_digits(char *)  */
DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(xdigit); /* size_t text_starts_with_xdigits(char *) */
DEF_CTYPE_SEQUENCE_PREFIX_MATCHER(alnum);  /* size_t text_starts_with_alnums(char *)  */

size_t text_starts_with_whitespace(char *src) {
  int p = 0;
  while (isspace(src[p++])) ;
  return p;
}

size_t text_starts_with_block_comment(char *src) {
  return text_has_delimited_prefix(src, "/*", "*/", 0);
}

size_t text_starts_with_line_comment(char *src) {
  int p = text_has_exact_prefix(src, "//");
  while (src[p] != '\n' && src[p] != '\0') p++;
  return p;
}

size_t text_starts_with_string_constant(char *src) {
  return text_has_delimited_prefix(src, "\"", "\"", 1)
      || text_has_delimited_prefix(src, "'", "'", 1);
}

size_t text_starts_with_interpolant(char *src) {
  return text_has_delimited_prefix(src, "#{", "}", 0);
}
