
/*
  This is when you want to compile a whole folder of stuff
*/
var opts = sass_new_context();
opts->sassPath = "/Users/hcatlin/dev/asset/stylesheet";
opts->cssPath = "/Users/hcatlin/dev/asset/stylesheets/.css";
opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
opts->outputStyle => SASS_STYLE_COMPRESSED;
sass_compile(opts, &callbackfunction);

/*
  This is when you want to compile a string
*/
opts = sass_new_context();
opts->inputString = "a { width: 50px; }";
opts->includePaths = "/Users/hcatlin/dev/asset/stylesheets:/Users/hcatlin/sasslib";
opts->outputStyle => SASS_STYLE_EXPANDED;
var cssResult = sass_compile(opts, &callbackfunction);