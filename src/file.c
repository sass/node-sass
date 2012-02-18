#include <stdlib.h>
#include <stdio.h>
#include "file.h"

char *sass_read_file(char *path) {
  FILE *f;
  f = fopen(path, "rb");
  if (!f) {
    printf("Couldn't open file %s", path);
    abort();
  }
  fseek(f, 0L, SEEK_END);
  int len = ftell(f);
  rewind(f);
  char *buf = (char *)malloc(len * sizeof(char) + 1);
  fread(buf, sizeof(char), len, f);
  buf[len] = '\0';
  return buf;
}