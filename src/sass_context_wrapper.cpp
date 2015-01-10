#include "sass_context_wrapper.h"

extern "C" {
  using namespace std;

  void compile_it(uv_work_t* req) {
    sass_context_wrapper* ctx_w = (sass_context_wrapper*)req->data;

    if (ctx_w->dctx) {
      compile_data(ctx_w->dctx);
    }
    else if (ctx_w->fctx) {
      compile_file(ctx_w->fctx);
    }
  }

  void compile_data(struct Sass_Data_Context* dctx) {
    sass_compile_data_context(dctx);
  }

  void compile_file(struct Sass_File_Context* fctx) {
    sass_compile_file_context(fctx);
  }

  sass_context_wrapper* sass_make_context_wrapper() {
    sass_context_wrapper* ctx_w = (sass_context_wrapper*)calloc(1, sizeof(sass_context_wrapper));

    ctx_w->importer_mutex = new std::mutex();
    ctx_w->importer_condition_variable = new std::condition_variable();

    return ctx_w;
  }

  void sass_wrapper_dispose(struct sass_context_wrapper* ctx_w, char* string = 0) {
    if (ctx_w->dctx) {
      sass_delete_data_context(ctx_w->dctx);
    }
    else if (ctx_w->fctx) {
      sass_delete_file_context(ctx_w->fctx);
    }

    delete ctx_w->file;
    delete ctx_w->prev;
    delete ctx_w->error_callback;
    delete ctx_w->success_callback;
    delete ctx_w->importer_callback;

    delete ctx_w->importer_mutex;
    delete ctx_w->importer_condition_variable;

    NanDisposePersistent(ctx_w->result);

    if(string) {
      free(string);
    }
  }

  void sass_free_context_wrapper(sass_context_wrapper* ctx_w) {
    sass_wrapper_dispose(ctx_w);

    free(ctx_w);
  }
}
