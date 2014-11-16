#include "sass_context_wrapper.h"

extern "C" {
  using namespace std;

  void compile_it(uv_work_t* req) {
    sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);

    if (ctx_w->dctx) {
      compile_data(ctx_w->dctx);
    } else if (ctx_w->fctx) {
      compile_file(ctx_w->fctx);
    }
  }

  void compile_data(struct Sass_Data_Context* dctx) {
    struct Sass_Context* ctx = sass_data_context_get_context(dctx);
    struct Sass_Options* ctx_opt = sass_context_get_options(ctx);
    sass_compile_data_context(dctx);
  }

  void compile_file(struct Sass_File_Context* fctx) {
    struct Sass_Context* ctx = sass_file_context_get_context(fctx);
    struct Sass_Options* ctx_opt = sass_context_get_options(ctx);
    sass_compile_file_context(fctx);
  }

  void free_data_context(struct Sass_Data_Context* dctx) {
   // delete[] dctx->source_string;
   // delete[] dctx->options.include_paths;
   // delete[] dctx->options.image_path;
    sass_delete_data_context(dctx);
  }

  void free_file_context(struct Sass_File_Context* fctx) {
   // delete[] fctx->input_path;
   // delete[] fctx->options.include_paths;
   // delete[] fctx->options.image_path;
    sass_delete_file_context(fctx);
  }

  sass_context_wrapper* sass_make_context_wrapper() {
    return (sass_context_wrapper*) calloc(1, sizeof(sass_context_wrapper));
  }

  void sass_free_context_wrapper(sass_context_wrapper* ctx_w) {
    if (ctx_w->dctx) {
      free_data_context(ctx_w->dctx);
    } else if (ctx_w->fctx) {
      free_file_context(ctx_w->fctx);
    }

    NanDisposePersistent(ctx_w->stats);
    delete ctx_w->callback;
    delete ctx_w->errorCallback;

    free(ctx_w);
  }
}
