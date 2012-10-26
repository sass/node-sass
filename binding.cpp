#include <v8.h>
#include <node.h>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "libsass/sass_interface.h"

using namespace v8;
using namespace std;

void WorkOnContext(uv_work_t* req) {
    sass_context* ctx = static_cast<sass_context*>(req->data);
    sass_compile(ctx);
}

void MakeCallback(uv_work_t* req) {
    HandleScope scope;
    TryCatch try_catch;
    sass_context* ctx = static_cast<sass_context*>(req->data);

    if (ctx->error_status == 0) {
        // if no error, do callback(null, result)
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
            Local<Value>::New(Null()),
            Local<Value>::New(String::New(ctx->output_string))
        };

        ctx->callback->Call(Context::GetCurrent()->Global(), argc, argv);
    } else {
        // if error, do callback(error)
        const unsigned argc = 1;
        Local<Value> argv[argc] = {
            Local<Value>::New(String::New(ctx->error_message))
        };

        ctx->callback->Call(Context::GetCurrent()->Global(), argc, argv);
    }
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    sass_free_context(ctx);
}

Handle<Value> Render(const Arguments& args) {
    HandleScope scope;
    sass_context* ctx = sass_new_context();
    String::AsciiValue astr(args[0]);
    Local<Function> callback = Local<Function>::Cast(args[1]);
    String::AsciiValue bstr(args[2]);

    ctx->source_string = new char[strlen(*astr)+1];
    strcpy(ctx->source_string, *astr);
    ctx->options.include_paths = new char[strlen(*bstr)+1];
    strcpy(ctx->options.include_paths, *bstr);
    // ctx->options.output_style = SASS_STYLE_NESTED;
    ctx->options.output_style = args[3]->Int32Value();
    ctx->callback = Persistent<Function>::New(callback);
    ctx->request.data = ctx;

    int status = uv_queue_work(uv_default_loop(), &ctx->request, WorkOnContext, MakeCallback);
    assert(status == 0);

    return Undefined();
}

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "render", Render);
}

NODE_MODULE(binding, RegisterModule);
