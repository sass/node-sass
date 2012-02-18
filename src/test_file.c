#include <stdio.h>
#include "file.h"

int main() {
  char *src = sass_read_file("chars.txt");
  printf("<BEGINNING OF FILE>\n");
  while (*src++) putchar(*src);
  printf("<EOF>");
  return 0;
}