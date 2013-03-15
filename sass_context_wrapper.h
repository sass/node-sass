#include "libsass/sass_interface.h"
#include <node.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sass_context_wrapper {
  sass_context* ctx;
  uv_work_t request;
  v8::Persistent<v8::Function> callback;
};

struct sass_context_wrapper*      sass_new_context_wrapper(void);
void sass_free_context_wrapper(struct sass_context_wrapper* ctx);

#ifdef __cplusplus
}
#endif
