#include <v8.h>
#include <node.h>
#include "libsass/sass_interface.h"

using namespace v8;

Handle<Value> Render(const Arguments& args) {
    HandleScope scope;
    struct sass_context* ctx = sass_new_context();
    String::AsciiValue  astr(args[0]); 
    char * cs = *astr;

    ctx->input_string = cs;
    ctx->options.include_paths = 0;
    ctx->options.output_style = SASS_STYLE_NESTED;

    sass_compile(ctx);

    return scope.Close(String::New(ctx->output_string));
}

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "render", Render);
}

NODE_MODULE(binding, RegisterModule);
