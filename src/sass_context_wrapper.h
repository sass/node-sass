#include <nan.h>
#include "libsass/sass_context.h"

#ifdef __cplusplus
extern "C" {
#endif

using namespace v8;

void compile_data(struct Sass_Data_Context* dctx);
void compile_file(struct Sass_File_Context* fctx);
void compile_it(uv_work_t* req);

struct sass_context_wrapper {
  Sass_Data_Context* dctx;
  Sass_File_Context* fctx;
  Persistent<Object> stats;
  uv_work_t request;
  NanCallback* success_callback;
  NanCallback* error_callback;
  NanCallback* importer_callback;
};

struct import_bag {
  const char* file;
  void* cookie;
  Sass_Import** incs;
};

struct sass_context_wrapper*      sass_make_context_wrapper(void);
void sass_free_context_wrapper(struct sass_context_wrapper* ctx_w);

#ifdef __cplusplus
}
#endif
