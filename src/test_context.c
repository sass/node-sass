
#include <stdio.h>
#include "context.h"

int main() {
  sass_context *ctx = sass_make_context_from_file("chars.txt");
  char *src = ctx->src;
  printf("<READING FROM FILE>\n");
  while (*src++) putchar(*src);
  printf("<EOF>\n");
  
  sass_context *ctx2 = sass_make_context_from_string(ctx->src);
  char *src2 = ctx2->src;
  printf("<READING FROM STRING>\n");
  while (*src2++) putchar(*src2);
  printf("<EOF>\n");
  
  return 0;
}