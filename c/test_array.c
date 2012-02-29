#include <stdio.h>
#include <stdlib.h>
#include "test_array.h"

DEFINE_SASS_ARRAY_OF(char);

int main() {
  sass_char_array a = sass_make_char_array(4);
  printf("%ld, %ld\n", a.length, a.capacity);
  sass_char_array_push(&a, 'h');
  sass_char_array_push(&a, 'e');
  sass_char_array_push(&a, 'l');
  sass_char_array_push(&a, 'l');
  
  printf("%ld, %ld\n", a.length, a.capacity);
  
  
  int i;
  for (i = 0; i < a.length; i++) putchar(a.contents[i]);
  putchar('\n');
  
  sass_char_array_push(&a, 'o');
  printf("%ld, %ld\n", a.length, a.capacity);
  
  for (i = 0; i < a.length; i++) putchar(a.contents[i]);
  putchar('\n');
  
  sass_char_array b = sass_make_char_array(7);
  sass_char_array_push(&a, ' ');
  sass_char_array_push(&a, 'w');
  sass_char_array_push(&a, 'o');
  sass_char_array_push(&a, 'r');
  sass_char_array_push(&a, 'l');
  sass_char_array_push(&a, 'd');
  
  sass_char_array_cat(&a, &b);
  
  printf("%ld, %ld\n", a.length, a.capacity);
  for (i = 0; i < a.length; i++) putchar(a.contents[i]);
  putchar('\n');
  
  return 0;
}