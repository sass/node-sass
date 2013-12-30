#include <v8.h>
#include <node.h>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "sass_context_wrapper.h"

using namespace v8;
using namespace std;


void WorkOnContext(uv_work_t* req) {
    sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
    if (ctx_w->ctx) {
      sass_context* ctx = static_cast<sass_context*>(ctx_w->ctx);
      sass_compile(ctx);
    } else if (ctx_w->fctx) {
      sass_file_context* ctx = static_cast<sass_file_context*>(ctx_w->fctx);
      sass_compile_file(ctx);
    }
}

void extractOptions(const Arguments& args, void* cptr, sass_context_wrapper* ctx_w, bool isFile) {
    char *source;
    char* pathOrData;
    int output_style;
    int source_comments;
    String::AsciiValue astr(args[0]);

    if (ctx_w) {
      // async (callback) style
      Local<Function> callback = Local<Function>::Cast(args[1]);
      Local<Function> errorCallback = Local<Function>::Cast(args[2]);
      if (isFile) {
        ctx_w->fctx = (sass_file_context*) cptr;
        ctx_w->callback = Persistent<Function>::New(callback);
        ctx_w->errorCallback = Persistent<Function>::New(errorCallback);
        ctx_w->request.data = ctx_w;
      } else {
        ctx_w->ctx = (sass_context*) cptr;
        ctx_w->callback = Persistent<Function>::New(callback);
        ctx_w->errorCallback = Persistent<Function>::New(errorCallback);
        ctx_w->request.data = ctx_w;
      }
      output_style = args[4]->Int32Value();
      source_comments = args[5]->Int32Value();
      String::AsciiValue bstr(args[3]);
      pathOrData = new char[strlen(*bstr)+1];
      strcpy(pathOrData, *bstr);
    } else {
      // synchronous style
      output_style = args[2]->Int32Value();
      source_comments = args[3]->Int32Value();
      String::AsciiValue bstr(args[1]);
      pathOrData = new char[strlen(*bstr)+1];
      strcpy(pathOrData, *bstr);
    }

    if (isFile) {
      sass_file_context *ctx = (sass_file_context*)cptr;
      char *filename = new char[strlen(*astr)+1];
      strcpy(filename, *astr);
      ctx->input_path = filename;
      ctx->options.image_path = new char[0];
      ctx->options.output_style = output_style;
      ctx->options.source_comments = source_comments;
      ctx->options.include_paths = pathOrData;
    } else {
      sass_context *ctx = (sass_context*)cptr;
      source = new char[strlen(*astr)+1];
      strcpy(source, *astr);
      ctx->source_string = source;
      ctx->options.image_path = new char[0];
      ctx->options.output_style = output_style;
      ctx->options.source_comments = source_comments;
      ctx->options.include_paths = pathOrData;
    }
}

void MakeCallback(uv_work_t* req) {
    HandleScope scope;
    TryCatch try_catch;
    sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
    sass_context* ctx = static_cast<sass_context*>(ctx_w->ctx);
    sass_file_context* fctx = static_cast<sass_file_context*>(ctx_w->fctx);

    if ((ctx && ctx->error_status == 0) || (fctx && fctx->error_status == 0)) {
        // if no error, do callback(null, result)
        const unsigned argc = 1;
        Local<Value> val;
        if (ctx) {
          val = Local<Value>::New(String::New(ctx->output_string));
        } else {
          val = Local<Value>::New(String::New(fctx->output_string));
        }
        Local<Value> argv[argc] = {val};
        ctx_w->callback->Call(Context::GetCurrent()->Global(), argc, argv);
    } else {
        // if error, do callback(error)
        const unsigned argc = 1;
        Local<Value> err;
        if (ctx) {
          err = Local<Value>::New(String::New(ctx->error_message));
        } else {
          err = Local<Value>::New(String::New(fctx->error_message));
        }
        Local<Value> argv[argc] = {err};
        ctx_w->errorCallback->Call(Context::GetCurrent()->Global(), argc, argv);
    }
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    if (fctx) {
      delete fctx->input_path;
    } else if (ctx) {
      delete ctx->source_string;
    }
    sass_free_context_wrapper(ctx_w);
}

Handle<Value> Render(const Arguments& args) {
    HandleScope scope;
    sass_context* ctx = sass_new_context();
    sass_context_wrapper* ctx_w = sass_new_context_wrapper();
    ctx_w->ctx = ctx;
    extractOptions(args, ctx, ctx_w, false);

    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, WorkOnContext, (uv_after_work_cb)MakeCallback);
    assert(status == 0);

    return scope.Close(Undefined());
}

Handle<Value> RenderSync(const Arguments& args) {
    HandleScope scope;
    sass_context* ctx = sass_new_context();
    extractOptions(args, ctx, NULL, false);

    sass_compile(ctx);

    delete ctx->source_string;
    ctx->source_string = NULL;
    delete ctx->options.include_paths;
    ctx->options.include_paths = NULL;

    if (ctx->error_status == 0) {
        Local<Value> output = Local<Value>::New(String::New(ctx->output_string));
        sass_free_context(ctx);
        return scope.Close(output);
    }

    Local<String> error = String::New(ctx->error_message);

    sass_free_context(ctx);
    ThrowException(Exception::Error(error));
    return scope.Close(Undefined());
}

Handle<Value> RenderFile(const Arguments& args) {
    HandleScope scope;
    sass_file_context* fctx = sass_new_file_context();
    sass_context_wrapper* ctx_w = sass_new_context_wrapper();
    ctx_w->fctx = fctx;
    extractOptions(args, fctx, ctx_w, true);

    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, WorkOnContext, (uv_after_work_cb)MakeCallback);
    assert(status == 0);

    return scope.Close(Undefined());
}

Handle<Value> RenderFileSync(const Arguments& args) {
    HandleScope scope;
    sass_file_context* ctx = sass_new_file_context();
    extractOptions(args, ctx, NULL, true);

    sass_compile_file(ctx);

    delete ctx->input_path;
    ctx->input_path = NULL;
    delete ctx->options.include_paths;
    ctx->options.include_paths = NULL;

    if (ctx->error_status == 0) {
        Local<Value> output = Local<Value>::New(String::New(ctx->output_string));
        sass_free_file_context(ctx);

        return scope.Close(output);
    }
    Local<String> error = String::New(ctx->error_message);
    sass_free_file_context(ctx);

    ThrowException(Exception::Error(error));
    return scope.Close(Undefined());
}

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "render", Render);
    NODE_SET_METHOD(target, "renderSync", RenderSync);
    NODE_SET_METHOD(target, "renderFile", RenderFile);
    NODE_SET_METHOD(target, "renderFileSync", RenderFileSync);
}

NODE_MODULE(binding, RegisterModule);
