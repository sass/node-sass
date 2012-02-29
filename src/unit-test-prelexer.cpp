#include <cstdio>
#include <iostream>
#include "prelexer.hpp"

using namespace Sass::Prelexer;
using std::cout;
using std::endl;

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
char ws1[]      = "  /* hello */\t\n//blah\n  /*blah*/ significant";

extern const char slash_star[] = "/*";

prelexer ptr = 0;
template <prelexer mx>
void try_and_set(char* src) {
  char* p = mx(src);
  if (p) ptr = mx;
}


int main() {
  
  prelexer p = exactly<'x'>;
  prelexer q = exactly<'x'>;
  
  if (p == q) {
    cout << "Hey, we can compare instantiated functions! And these are the same!" << endl;
  }
  
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
  check_twice(spaces_and_comments, ws1, num1);
  cout << count_interval<'\n'>(ws1, spaces_and_comments(ws1)) << endl;
  cout << count_interval<'*'>(ws1, spaces_and_comments(ws1)) << endl;
  cout << count_interval<exactly<slash_star> >(ws1, spaces_and_comments(ws1)) << endl;
  putchar(*(find_first<'{'>(pre1))), putchar('\n');
  
  try_and_set<exactly<'-'> >(ident1);
  prelexer ptr1 = ptr;
  try_and_set<exactly<'@'> >(atkwd1);
  prelexer ptr2 = ptr;
  cout << ptr << endl;
  if (ptr1 == ptr2) cout << "This shouldn't be the case!" << endl;
  else cout << "The prelexer pointers are different!" << endl;
  
  //ptr == prelexer(exactly<'x'>);

  return 0;
}