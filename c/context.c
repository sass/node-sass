#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "context.h"

static sass_context *sass_alloc_context() {
  sass_context *ctx = malloc(sizeof(sass_context));
  if (!ctx) {
    printf("ERROR: could not allocate Sass context object.\n");
    abort();
  }
  return ctx;
}

static char *sass_read_file(char *path) {
  FILE *f;
  f = fopen(path, "rb");
  if (!f) {
    printf("ERROR: could not open Sass source file %s", path);
    abort();
  }
  fseek(f, 0L, SEEK_END);
  int len = ftell(f);
  rewind(f);
  char *buf = (char *)malloc(len * sizeof(char) + 1);
  fread(buf, sizeof(char), len, f);
  buf[len] = '\0';
  fclose(f);
  return buf;
}

sass_context *sass_make_context_from_file(char *path) {
  sass_context *ctx = sass_alloc_context();
  ctx->path = path;
  ctx->pos = ctx->src = sass_read_file(path);
  ctx->line = 1;
  return ctx;
}

sass_context *sass_make_context_from_string(char *src) {
  size_t len = strlen(src);
  sass_context *ctx = sass_alloc_context();
  if (!(ctx->pos = ctx->src = (char *) malloc(len * sizeof(char) + 1) )) {
    printf("ERROR: could not copy Sass source string.\n");
    abort();
  }
  memcpy(ctx->src, src, len);
  ctx->src[len] = '\0';
  ctx->line = 1;
  return ctx;
} 

void sass_free_context(sass_context *ctx) {
  free(ctx->src);
  free(ctx);
}

