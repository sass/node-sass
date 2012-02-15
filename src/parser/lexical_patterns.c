#include "prefix_primitives.h"

int prefix_starts_with_identifier(char *src) {
  int p = prefix_is_alphas(src) || prefix_is_exactly(src, "_");
  if (!p++) return 0;
  while (src[p]) {
    int q = prefix_is_alnums(src);
    q = q ? q : prefix_is_exactly(src, "_");
    if (!q) return p;
    p += q;
  }
}
    

DEFINE_EXACT_MATCHER(star, "*");

DEFINE_EXACT_MATCHER(dot, ".");
DEFINE_EXACT_MATCHER(hash, "#");

DEFINE_EXACT_MATCHER(adjacent_to, "+");
DEFINE_EXACT_MATCHER(precedes, "~");
DEFINE_EXACT_MATCHER(parent_of, ">");
prefix_matcher prefix_starts_with_ancestor_of = prefix_starts_with_spaces;
DEFINE_EXACT_MATCHER(exclamation, "!");
DEFINE_EXACT_MATCHER(dollar, "$");
DEFINE_EXACT_MATCHER(percent, "%");
DEFINE_EXACT_MATCHER(ampersand, "&");
DEFINE_EXACT_MATCHER(times, "*");
DEFINE_EXACT_MATCHER(comma, ",");
DEFINE_EXACT_MATCHER(hyphen, "-");
DEFINE_EXACT_MATCHER(minus, "-");
DEFINE_EXACT_MATCHER(slash, "/");
DEFINE_EXACT_MATCHER(divide, "/");
DEFINE_EXACT_MATCHER(colon, ":");
DEFINE_EXACT_MATCHER(semicolon, ";");
DEFINE_EXACT_MATCHER(lt, "<");
DEFINE_EXACT_MATCHER(lte, "<=");
DEFINE_EXACT_MATCHER(gt, ">");
DEFINE_EXACT_MATCHER(parent_of, ">");
DEFINE_EXACT_MATCHER(gte, ">=");
