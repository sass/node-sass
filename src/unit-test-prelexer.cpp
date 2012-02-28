#include <cstdio>
#include "prelexer.hpp"

using namespace Sass::Prelexer;

void print_slice(const char *s, const char *t) {
  if (t) {
    printf("succeeded with %ld characters:\t", t - s);
    while (s < t) putchar(*s++);
    putchar('\n');
  }
  else {
    printf("failed\n");
  }
}

#define check_twice(matcher, inp1, inp2) \
  (printf("Attempting to match %s\n", #matcher), \
   print_slice(inp1, matcher(inp1)), \
   print_slice(inp2, matcher(inp2)), \
   putchar('\n'))

char num1[]     = "123foogoo";
char num2[]     = ".456e7";
char num3[]     = "-23.45";
char ident1[]   = "-webkit-box-sizing: border-box;";
char interp1[]  = "#{$stuff + $more_stuff}";
char atkwd1[]   = "@media screen blah blah { ... }";
char idnm1[]    = "#-blah_hoo { color: red; }";
char classnm1[] = ".hux-boo23 { color: blue; }";
char percent1[] = "110%; color: red; }";
char dim1[]     = "12px; color: blue; }";
char uri1[]     = "url(  'www.bork.com/bork.png' )";
char ident2[]   = "url ()// not a uri!";
char func1[]    = "lighten(indigo, 20%)";
char exact1[]   = "='blah'] { ... }";
char inc1[]     = "~='foo'] { ... }";
char dash1[]    = "|='bar'] { ... }";
char pre1[]     = "^='hux'] { ... }";
char suf1[]     = "$='baz'] { ... }";
char sub1[]     = "*='bum'] { ... }";

int main() {
  
  check_twice(identifier, ident1, num1);
  check_twice(interpolant, interp1, idnm1);
  check_twice(at_keyword, atkwd1, classnm1);
  check_twice(id_name, idnm1, interp1);
  check_twice(class_name, classnm1, num2);
  check_twice(number, num1, ident1);
  check_twice(number, num2, classnm1);
  check_twice(number, num3, ident1);
  check_twice(percentage, percent1, dim1);
  check_twice(dimension, dim1, percent1);
  check_twice(uri, uri1, ident2);
  check_twice(function, func1, ident2);
  check_twice(exact_match, exact1, inc1);
  check_twice(class_match, inc1, pre1);
  check_twice(dash_match, dash1, suf1);
  check_twice(prefix_match, pre1, dash1);
  check_twice(suffix_match, suf1, pre1);
  check_twice(substring_match, sub1, suf1);

  return 0;
}