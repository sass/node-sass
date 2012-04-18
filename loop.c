#include <stdio.h>
#include "sass_interface.h"

int main(int argc, char** argv)
{	
	if (argc < 2) {
		printf("Hey, I need an input file!\n");
		return 0;
	}
	
  while (1) {
    struct sass_file_context* ctx = sass_new_file_context();
    ctx->options.include_paths = "::/blah/bloo/fuzz:/slub/flub/chub::/Users/Aaron/dev/libsass/::::/huzz/buzz:::";
    ctx->options.output_style = SASS_STYLE_NESTED;
    ctx->input_path = argv[1];

    sass_compile_file(ctx);

    if (ctx->error_status) {
      if (ctx->error_message) printf("%s", ctx->error_message);
      else printf("An error occured; no error message available.\n");
      break;
    }
    else if (ctx->output_string) {
      continue;
    }
    else {
      printf("Unknown internal error.\n");
      break;
    }

    sass_free_file_context(ctx);
  }
	return 0;
}