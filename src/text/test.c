#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prefix_primitives.h"

#define test1(name, v1) t1(#name, name, v1)
#define test2(name, v1, v2) t2(#name, name, v1, v2)

void print_slice(char *s, char *t) {
  if (t) {
    printf("matched %ld characters:\t", t - s);
    while (s < t) putchar(*s++);
    putchar('\n');
  }
  else {
    printf("matched nothing\n");
  }
}

void t1(char *name, char *(*matcher)(char *), char *src) {
  printf("testing %s(\"%s\")\n", name, src);
  print_slice(src, matcher(src));
  putchar('\n');
}

void t2(char *name, char *(*matcher)(char *, char *), char *src, char *pre) {
  printf("testing %s(\"%s\", \"%s\")\n", name, src, pre);
  print_slice(src, matcher(src, pre));
  putchar('\n');
}

int main() {
  char *r = "\"blah blah \\\" blah\"";
  char *s = "'this \\'is\\' a \"string\" now' blah blah blah";
  char *t = "/* this is a c comment \\*/ blah blah";
  char *u = "#{ this is an interpolant \\} blah blah";
  char *v = "hello my name is aaron";
  char *w = "_identifier123";
  char *x = "12non_ident_ifier_";
  char *y = "-blah-blah_blah";
  char *z = "#foo > :first-child { color: #abcdef; }";
  char *line_comment = "// blah blah blah // end\n blah blah";
  char *stuff = "badec4669264hello";
  
  // test2(prefix_is_char, v, 'h');
  // test2(prefix_is_char, v, 'a');
  
  print_slice(v, prefix_is_char(v, 'h'));
  print_slice(v, prefix_is_char(v, 'a'));
  putchar('\n');
  
  test2(prefix_is_chars, v, "hello");
  test2(prefix_is_chars, v, "hello world");

  test2(prefix_is_one_of, v, "abcdefgh");
  test2(prefix_is_one_of, v, "ijklmnop");
  
  test2(prefix_is_some_of, w, "_deint");
  test2(prefix_is_some_of, w, "abcd");
  
  test1(prefix_is_block_comment, t);
  test1(prefix_is_block_comment, line_comment);
  
  test1(prefix_is_double_quoted_string, r);
  test1(prefix_is_double_quoted_string, s);
  
  test1(prefix_is_single_quoted_string, s);
  test1(prefix_is_single_quoted_string, r);

  test1(prefix_is_interpolant, u);
  test1(prefix_is_interpolant, z);
  
  test1(prefix_is_line_comment, line_comment);
  test1(prefix_is_line_comment, t);
  
  char *p = prefix_sequence(stuff, prefix_is_alphas, prefix_is_digits);
  
  print_slice(stuff, _prefix_sequence(stuff, prefix_is_alphas, prefix_is_digits, NULL));
  print_slice(stuff, prefix_sequence(stuff, prefix_is_alphas, prefix_is_puncts));
  
  // printn(s, prefix_is_string(s));
  // printn(s, prefix_is_one_of(s, "abcde+'"));
  // printn(s, prefix_is_some_of(s, "'abcdefghijklmnopqrstuvwxyz "));
  // printn(t, prefix_is_block_comment(t));
  // printn(u, prefix_is_interpolant(u));
  // printn(v, prefix_is_alphas(v));
  // printn(v, prefix_is_alpha(v));
  // printn(v, prefix_is_exactly(v, "hello"));
  // printn(x, prefix_sequence(x, prefix_is_digits, prefix_is_alphas));
  // printn(x, prefix_alternatives(x, prefix_is_hyphen, prefix_is_alphas, prefix_is_puncts, prefix_is_digits));

  return 0;
}
