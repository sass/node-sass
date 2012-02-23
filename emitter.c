
#include "emitter.h"

int sass_emit(sass_context *ctx) {
  ctx->result = ctx->doc->src;
  return 0;
}