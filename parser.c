
#include "parser.h"
#include <stdlib.h>

int sass_parse(sass_context *ctx) {
  sass_document *doc = (sass_document*)malloc(sizeof(sass_document));
  doc->src = ctx->src;
  return 0;
}