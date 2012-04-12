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

struct sass_context* sass_new_context();

char* sass_compile(struct sass_context*);

#ifdef __cplusplus
}
#endif