#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prefix.h"

void print_slice(char *s, char *t) {
  if (t) {
    printf("succeeded with %ld characters:\t", t - s);
    while (s < t) putchar(*s++);
    putchar('\n');
  }
  else {
    printf("failed\n");
  }
}

#define test1(matcher, src) \
(printf("testing << %s >>\n", #matcher), print_slice(src, matcher(src)))

#define testn(matcher, src, ...) \
(printf("testing << %s >>\n", #matcher), print_slice(src, matcher(src, __VA_ARGS__)))

int main() {
  char *spaces = "  \t \t \v \r\n  \n\nhello world";
  char *dqstring = "\"blah blah \\\" blah\"";
  char *sqstring = "'this \\'is\\' a \"string\" now' blah blah blah";
  char *scomment = "# a shell-style comment";
  char *bcomment = "/* this is a c comment \\*/ blah blah";
  char *non_comment = "/* blah blah";
  char *interpolant = "#{ this is an interpolant \\} blah blah";
  char *words = "hello my name is aaron";
  char *id1 = "_identifier123{blah bloo}";
  char *non_id = "12non_ident_ifier_";
  char *word2 = "-blah-bl+ah_bl12-34+1:foobar;";
  char *non_word = "-12blah-bloo";
  char *selector = "#foo > :first-child { color: #abcdef; }";
  char *lcomment = "// blah blah blah // end\n blah blah";
  char *id2 = "badec4669264hello";
  char *integer1 = "3837483+3";
  char *integer2 = "+294739-4";
  char *integer3 = "-294729+1";
  char *class = ".blah-blah_bloo112-blah+blee4 hello";
  char *id = "#foo_bar-baz123-hux blee";
  char *var = "$blah123-blah";
  char *non_var = "$ hux";
  
  test1(sass_prefix_is_spaces, spaces);
  test1(sass_prefix_is_spaces, words);
  
  testn(sass_prefix_is_char, words, 'h');
  testn(sass_prefix_is_char, words, 'a');
  
  testn(sass_prefix_is_chars, words, "hello");
  testn(sass_prefix_is_chars, words, "hello world");
  
  testn(sass_prefix_is_one_of, words, "abcdefgh");
  testn(sass_prefix_is_one_of, words, "ijklmnop");
  
  testn(sass_prefix_is_some_of, id1, "_deint");
  testn(sass_prefix_is_some_of, id1, "abcd");

  test1(sass_prefix_is_block_comment, bcomment);
  test1(sass_prefix_is_block_comment, non_comment);
  
  test1(sass_prefix_is_double_quoted_string, dqstring);
  test1(sass_prefix_is_double_quoted_string, sqstring);

  test1(sass_prefix_is_single_quoted_string, sqstring);
  test1(sass_prefix_is_single_quoted_string, dqstring);

  test1(sass_prefix_is_interpolant, interpolant);
  test1(sass_prefix_is_interpolant, lcomment);
  
  test1(sass_prefix_is_line_comment, lcomment);
  test1(sass_prefix_is_line_comment, non_comment);
  
  test1(sass_prefix_epsilon, words);
  
  testn(sass_prefix_not, words, sass_prefix_is_puncts);
  testn(sass_prefix_not, words, sass_prefix_is_alphas);
  
  testn(sass_prefix_sequence, id2, sass_prefix_is_alphas, sass_prefix_is_digits);
  testn(sass_prefix_sequence, id2, sass_prefix_is_alphas, sass_prefix_is_puncts);
  
  testn(sass_prefix_optional, non_id, sass_prefix_is_digits);
  testn(sass_prefix_optional, words, sass_prefix_is_digits);
  
  testn(sass_prefix_zero_plus, words, sass_prefix_is_alphas);
  testn(sass_prefix_zero_plus, non_id, sass_prefix_is_alphas);
  
  testn(sass_prefix_one_plus, words, sass_prefix_is_alphas);
  testn(sass_prefix_one_plus, non_id, sass_prefix_is_alphas);
  
  test1(sass_prefix_is_classname, class);
  test1(sass_prefix_is_classname, words);
  
  test1(sass_prefix_is_idname, id);
  test1(sass_prefix_is_idname, class);
  
  test1(sass_prefix_is_variable, var);
  test1(sass_prefix_is_variable, non_var);


  return 0;
}
