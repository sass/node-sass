#ifdef __cplusplus
extern "C" {
#endif

struct sass_context {
  char* sass_path;
  char* css_path;
  char* include_paths;
  char* input_file;
  char* input_string;
  unsigned int output_style;
};

sass_context* sass_new_context();

char* sass_compile(sass_context* c_ctx);

#ifdef __cplusplus
}
#endif