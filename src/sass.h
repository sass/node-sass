#define SASS

#ifdef __cplusplus
extern "C" {
#endif

struct Sass_Context {
  const char*  input_path;
  const char*  input_string;
  char*        output_string;

  int          error_status;
  char*        error_message;

  int          output_style;
  int          source_comments;
  int          source_maps;
  const char*  image_path;
  const char*  include_paths_string;
  const char** include_paths_array;
};

struct Sass_Context* make_sass_context();
void free_sass_context(struct Sass_Context*);
void compile_sass_file(struct Sass_Context*);
void compile_sass_string(struct Sass_Context*);

#ifdef __cplusplus
}
#endif
