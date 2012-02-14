#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prefix_primitives.h"


void printn(char *s, int n) {
  int i;
  printf("matched %d characters:\t", n);
  for (i = 0; i < n; i++) {
    putchar(s[i]);
  }
  putchar('\n');
}

int main() {
  char *s = "'this \\'is\\' a \"string\" now' blah blah blah";
  char *t = "/* this is a c comment \\*/ blah blah";
  char *u = "#{ this is an interpolant \\} blah blah";
  char *v = "hello my name is aaron";
  
  int (*funcs[])(char *) = { prefix_is_string, prefix_is_block_comment, NULL };

  printn(s, funcs[0](s));
  printn(s, prefix_is_one_of(s, "abcde+'"));
  printn(s, prefix_is_some_of(s, "'abcdefghijklmnopqrstuvwxyz "));
  printn(t, funcs[1](t));
  printn(u, prefix_is_interpolant(u));
  printn(v, prefix_is_alphas(v));
  printn(v, prefix_is_one_alpha(v));
  printn(v, prefix_is_exactly(v, "hello"));
  
  {
    int hidden = 42;
    printf("%d\n", hidden);
  }
  
  printf("%s", "Did this run?\n");
  
  return 0;
}
