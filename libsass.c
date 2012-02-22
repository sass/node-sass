
#include "libsass.h"

char * sass_file_compile(char *filepath, int options) {
	sass_context *ctx = sass_make_context_from_file(filepath);
	return ctx->src;
}

char * sass_string_compile(char *input, int options) {
	sass_context *ctx = sass_make_context_from_string(input);
	return ctx->src;
}