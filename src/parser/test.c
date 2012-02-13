#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prefix_primitives.h"

int main() {
  char *s = "'this is a \"string\" now' blah blah blah";
  int l = prefix_is_string_constant(s);
  if (l) {
    printf("matched a string literal of length %d:\n", l);
    int i;
    for (i = 0; i < l; i++) {
      putchar(s[i]);
    }
    putchar('\n');
  }
  else {
    printf("matched %d characters\n", l);
  }
  
  unsigned char x;
  printf("By the way, punctuation symbols are:\n");
  for (x = '\0'; x < 128; x++) if (ispunct(x)) printf("%c", x);
  putchar('\n');
  printf("By the way, 0 || 24 is: %d\n", 0 || 24);
  printf("And 24 || 0 is: %d\n", 24 || 0);
  return 0;
}