#ifdef __cplusplus
extern "C" {
#endif

#define SASS_STYLE_NESTED     0;
#define SASS_STYLE_EXPANDED   1;
#define SASS_STYLE_COMPACT    2;
#define SASS_STYLE_COMPRESSED 3;

struct sass_options {
  int output_style;
  char* include_paths;
};

struct sass_context {
  char* input_string;
  char* output_string;
  struct sass_options options;
};

struct sass_folder_context {
  char* search_path;
  char* output_path;
  struct sass_options options;
};

struct sass_file_context {
  char* input_path;
  char* output_string;
  struct sass_options options;
};

struct sass_context*        sass_new_context        ();
struct sass_folder_context* sass_new_folder_context ();
struct sass_file_context*   sass_new_file_context   ();

int sass_compile        (struct sass_context*);
// int sass_folder_compile (struct sass_folder_context*);
int sass_file_compile   (struct sass_file_context*);

#ifdef __cplusplus
}
#endif