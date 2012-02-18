#include <stdlib.h>
#include <stdio.h>
#include "context.h"
#include "file.h"

sass_context *make_sass_context_from_file(char *path) {
  sass_context *ctx = malloc(sizeof(sass_context));
  if (!ctx) {
    printf("ERROR: could not allocate sass context object.\n");
    abort();
  }
  ctx->path = path;
  ctx->src = sass_read_file(path);
  return ctx;
}