#include <nan.h>
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

char* CreateString(Local<Value> value) {
  size_t count;
  return NanCString(value, &count);
}

void ExtractOptions(Local<Value> optionsValue, void* cptr, sass_context_wrapper* ctx_w, bool isFile) {
  int source_comments;
  Local<Object> options = Local<Object>::Cast(optionsValue);

  if (ctx_w) {
    NanInitPersistent(Object, stats, Local<Object>::Cast(options->Get(NanSymbol("stats"))));

    // async (callback) style
    Local<Function> callback = Local<Function>::Cast(options->Get(NanSymbol("success")));
    Local<Function> errorCallback = Local<Function>::Cast(options->Get(NanSymbol("error")));
    if (isFile) {
      ctx_w->fctx = (sass_file_context*) cptr;
    } else {
      ctx_w->ctx = (sass_context*) cptr;
    }
    ctx_w->stats = stats;
    ctx_w->request.data = ctx_w;
    ctx_w->callback = new NanCallback(callback);
    ctx_w->errorCallback = new NanCallback(errorCallback);
  }

  if (isFile) {
    sass_file_context* ctx = (sass_file_context*) cptr;
    ctx->input_path = CreateString(options->Get(NanSymbol("file")));
    ctx->options.image_path = CreateString(options->Get(NanSymbol("imagePath")));
    ctx->options.output_style = options->Get(NanSymbol("style"))->Int32Value();
    ctx->options.source_comments = source_comments = options->Get(NanSymbol("comments"))->Int32Value();
    ctx->options.include_paths = CreateString(options->Get(NanSymbol("paths")));
    if (source_comments == SASS_SOURCE_COMMENTS_MAP) {
      ctx->source_map_file = CreateString(options->Get(NanSymbol("sourceMap")));
    }
  } else {
    sass_context* ctx = (sass_context*) cptr;
    ctx->source_string = CreateString(options->Get(NanSymbol("data")));
    ctx->options.image_path = CreateString(options->Get(NanSymbol("imagePath")));
    ctx->options.output_style = options->Get(NanSymbol("style"))->Int32Value();
    ctx->options.source_comments = source_comments = options->Get(NanSymbol("comments"))->Int32Value();
    ctx->options.include_paths = CreateString(options->Get(NanSymbol("paths")));
  }
}

template<typename Ctx>
void FillStatsObj(Handle<Object> stats, Ctx ctx) {
  int i;
  Handle<Array> arr;

  arr = Array::New(ctx->num_included_files);
  for (i = 0; i < ctx->num_included_files; i++) {
    arr->Set(i, String::New(ctx->included_files[i]));
  }
  (*stats)->Set(NanSymbol("includedFiles"), arr);
}

void FillStatsObj(Handle<Object> stats, sass_file_context* ctx) {
  Handle<Value> source_map;

  FillStatsObj<sass_file_context*>(stats, ctx);
  if (ctx->options.source_comments == SASS_SOURCE_COMMENTS_MAP) {
    source_map = String::New(ctx->source_map_string);
  } else {
    source_map = Null();
  }
  (*stats)->Set(NanSymbol("sourceMap"), source_map);
}

void MakeCallback(uv_work_t* req) {
  NanScope();

  TryCatch try_catch;
  sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
  Handle<Value> val, err;
  int error_status = ctx_w->ctx ? ctx_w->ctx->error_status : ctx_w->fctx->error_status;

  if (ctx_w->ctx) {
    FillStatsObj(ctx_w->stats, ctx_w->ctx);
  } else {
    FillStatsObj(ctx_w->stats, ctx_w->fctx);
  }

  if (error_status == 0) {
    // if no error, do callback(null, result)
    Handle<Value> source_map;
    if (ctx_w->fctx && ctx_w->fctx->options.source_comments == SASS_SOURCE_COMMENTS_MAP) {
      source_map = String::New(ctx_w->fctx->source_map_string);
    } else {
      source_map = Null();
    }

    val = ctx_w->ctx ? NanNewLocal(String::New(ctx_w->ctx->output_string)) : NanNewLocal(String::New(ctx_w->fctx->output_string));
    Local<Value> argv[] = {
      NanNewLocal(val),
      NanNewLocal(source_map)
    };
    ctx_w->callback->Call(2, argv);
  } else {
    // if error, do callback(error)
    err = ctx_w->ctx ? NanNewLocal(String::New(ctx_w->ctx->error_message)) : NanNewLocal(String::New(ctx_w->fctx->error_message));
    Local<Value> argv[] = {
      NanNewLocal(err),
      NanNewLocal(Integer::New(error_status))
    };
    ctx_w->errorCallback->Call(2, argv);
  }
  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }
  sass_free_context_wrapper(ctx_w);
}

NAN_METHOD(Render) {
  NanScope();

  sass_context* ctx = sass_new_context();
  sass_context_wrapper* ctx_w = sass_new_context_wrapper();
  ctx_w->ctx = ctx;
  ExtractOptions(args[0], ctx, ctx_w, false);

  int status = uv_queue_work(uv_default_loop(), &ctx_w->request, WorkOnContext, (uv_after_work_cb)MakeCallback);
  assert(status == 0);

  NanReturnUndefined();
}

NAN_METHOD(RenderSync) {
  NanScope();
  Handle<Object> options = args[0]->ToObject();
  sass_context* ctx = sass_new_context();
  ExtractOptions(args[0], ctx, NULL, false);

  sass_compile(ctx);

  FillStatsObj(options->Get(NanSymbol("stats"))->ToObject(), ctx);

  if (ctx->error_status == 0) {
    Local<Value> output = NanNewLocal(String::New(ctx->output_string));
    free_context(ctx);
    NanReturnValue(output);
  }

  Local<String> error = String::New(ctx->error_message);
  free_context(ctx);
  NanThrowError(error);
  NanReturnUndefined();
}

NAN_METHOD(RenderFile) {
  NanScope();
  sass_file_context* fctx = sass_new_file_context();
  sass_context_wrapper* ctx_w = sass_new_context_wrapper();
  ctx_w->fctx = fctx;
  ExtractOptions(args[0], fctx, ctx_w, true);

  int status = uv_queue_work(uv_default_loop(), &ctx_w->request, WorkOnContext, (uv_after_work_cb)MakeCallback);
  assert(status == 0);

  NanReturnUndefined();
}

NAN_METHOD(RenderFileSync) {
  NanScope();
  sass_file_context* ctx = sass_new_file_context();
  ExtractOptions(args[0], ctx, NULL, true);
  Handle<Object> options = args[0]->ToObject();

  sass_compile_file(ctx);

  FillStatsObj(options->Get(NanSymbol("stats"))->ToObject(), ctx);

  if (ctx->error_status == 0) {
    Local<Value> output = NanNewLocal(String::New(ctx->output_string));
    free_file_context(ctx);
    NanReturnValue(output);
  }

  Local<String> error = String::New(ctx->error_message);
  free_file_context(ctx);
  NanThrowError(error);
  NanReturnUndefined();
}

void RegisterModule(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "render", Render);
  NODE_SET_METHOD(target, "renderSync", RenderSync);
  NODE_SET_METHOD(target, "renderFile", RenderFile);
  NODE_SET_METHOD(target, "renderFileSync", RenderFileSync);
}

NODE_MODULE(binding, RegisterModule);
