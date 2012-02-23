
#include "libsass.h"
#include "emitter.h"
#include "transforms.h"
#include "parser.h"

char * sass_file_compile(char *filepath, int options) {
	sass_context *ctx = sass_make_context_from_file(filepath);
	return ctx->src;
}

char * sass_string_compile(char *input, int options) {
	sass_context *ctx = sass_make_context_from_string(input);
	return ctx->src;
}

char * sass_compile(sass_context *ctx) {
  sass_parse(ctx);
  sass_transform(ctx);
  sass_emit(ctx);
  return ctx->result;
}