#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "text_prefix_matchers.h"

int main() {
  char *s = "'this is a \"string\" now' blah blah blah";
  int l = text_has_delimited_prefix(s, "'", "'", 1);
  if (l) {
    printf("matched a string literal of length %d:\n", l);
    int i;
    for (i = 0; i < l; i++) {
      putchar(s[i]);
    }
    putchar('\n');
  }
  return 0;
}