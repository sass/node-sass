// 
//   This is when you want to compile a whole folder of stuff
// 
// var opts = sass_new_context();
// opts->sassPath = "/Users/hcatlin/dev/asset/stylesheet";
// opts->cssPath = "/Users/hcatlin/dev/asset/stylesheets/.css";
// opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
// opts->outputStyle => SASS_STYLE_COMPRESSED;
// sass_compile(opts, &callbackfunction);
// 
// 
//   This is when you want to compile a string
// 
// opts = sass_new_context();
// opts->inputString = "a { width: 50px; }";
// opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
// opts->outputStyle => SASS_STYLE_EXPANDED;
// var cssResult = sass_compile(opts, &callbackfunction);

#include <stdio.h>
#include "sass_interface.h"

int main(int argc, char** argv)
{	
	if (argc < 2) {
		printf("Hey, I need an input file!\n");
		return 0;
	}
	
	struct sass_file_context* ctx = sass_new_file_context();
	ctx->options.include_paths = "::/blah/bloo/fuzz:/slub/flub/chub::/Users/Aaron/dev/libsass/::::/huzz/buzz:::";
	ctx->options.output_style = SASS_STYLE_NESTED;
	ctx->input_path = argv[1];
		
	sass_compile_file(ctx);
	
	printf("%s", ctx->output_string);
	
  sass_free_file_context(ctx);
	return 0;
}