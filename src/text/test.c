#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
// #include "prefix_primitives.h"
#include "lexical_patterns.h"

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
  
  test1(prefix_is_spaces, spaces);
  test1(prefix_is_spaces, words);
  
  testn(prefix_is_char, words, 'h');
  testn(prefix_is_char, words, 'a');
  
  testn(prefix_is_chars, words, "hello");
  testn(prefix_is_chars, words, "hello world");
  
  testn(prefix_is_one_of, words, "abcdefgh");
  testn(prefix_is_one_of, words, "ijklmnop");
  
  testn(prefix_is_some_of, id1, "_deint");
  testn(prefix_is_some_of, id1, "abcd");
  
  test1(prefix_is_shell_comment, scomment);
  test1(prefix_is_shell_comment, lcomment);

  test1(prefix_is_c_block_comment, bcomment);
  test1(prefix_is_c_block_comment, non_comment);
  
  test1(prefix_is_double_quoted_string, dqstring);
  test1(prefix_is_double_quoted_string, sqstring);

  test1(prefix_is_single_quoted_string, sqstring);
  test1(prefix_is_single_quoted_string, dqstring);

  test1(prefix_is_interpolant, interpolant);
  test1(prefix_is_interpolant, lcomment);
  
  test1(prefix_is_c_line_comment, lcomment);
  test1(prefix_is_c_line_comment, non_comment);
  
  test1(prefix_epsilon, words);
  
  testn(prefix_not, words, prefix_is_puncts);
  testn(prefix_not, words, prefix_is_alphas);
  
  testn(prefix_sequence, id2, prefix_is_alphas, prefix_is_digits);
  testn(prefix_sequence, id2, prefix_is_alphas, prefix_is_puncts);
  
  testn(prefix_optional, non_id, prefix_is_digits);
  testn(prefix_optional, words, prefix_is_digits);
  
  testn(prefix_zero_plus, words, prefix_is_alphas);
  testn(prefix_zero_plus, non_id, prefix_is_alphas);
  
  testn(prefix_one_plus, words, prefix_is_alphas);
  testn(prefix_one_plus, non_id, prefix_is_alphas);
  
  test1(prefix_is_identifier, id1);
  test1(prefix_is_identifier, non_id);
  
  test1(prefix_is_integer, integer1);
  test1(prefix_is_integer, integer2);
  test1(prefix_is_integer, integer3);
  test1(prefix_is_integer, word2);
  
  test1(prefix_is_word, word2);
  test1(prefix_is_word, integer3);
  
  test1(prefix_is_class, class);
  test1(prefix_is_class, words);
  
  test1(prefix_is_id, id);
  test1(prefix_is_id, class);
  
  testn(prefix_find_first, word2, prefix_is_semicolon_or_lbrack);

  return 0;
}
