#include <stdlib.h>
#include <nan.h>
#include <condition_variable>
#include "libsass/sass_context.h"

#ifdef __cplusplus
extern "C" {
#endif

  using namespace v8;

  void compile_data(struct Sass_Data_Context* dctx);
  void compile_file(struct Sass_File_Context* fctx);
  void compile_it(uv_work_t* req);

  struct sass_context_wrapper {
    // binding related
    bool is_sync;
    void* cookie;
    const char* prev;
    const char* file;
    std::mutex* importer_mutex;
    std::condition_variable* importer_condition_variable;

    // libsass related
    Sass_Import** imports;
    Sass_Data_Context* dctx;
    Sass_File_Context* fctx;

    // libuv related
    uv_async_t async;
    uv_work_t request;

    // v8 and nan related
    Persistent<Object> result;
    NanCallback* error_callback;
    NanCallback* success_callback;
    NanCallback* importer_callback;
  };

  struct sass_context_wrapper*      sass_make_context_wrapper(void);
  void sass_wrapper_dispose(struct sass_context_wrapper*, char*);
  void sass_free_context_wrapper(struct sass_context_wrapper*);

#ifdef __cplusplus
}
#endif
