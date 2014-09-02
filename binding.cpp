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
  if(value->IsNull() || !value->IsString()) {
    return const_cast<char*>(""); // return empty string.
  }

  String::Utf8Value string(value);
  char *str = (char *) malloc(string.length() + 1);
  strcpy(str, *string);
  return str;
}

void ExtractOptions(Local<Value> optionsValue, void* cptr, sass_context_wrapper* ctx_w, bool isFile) {
  int source_comments;
  Local<Object> options = optionsValue->ToObject();

  if (ctx_w) {
    NanAssignPersistent(ctx_w->stats, options->Get(NanNew("stats"))->ToObject());

    // async (callback) style
    Local<Function> callback = Local<Function>::Cast(options->Get(NanNew("success")));
    Local<Function> errorCallback = Local<Function>::Cast(options->Get(NanNew("error")));
    if (isFile) {
      ctx_w->fctx = (sass_file_context*) cptr;
    } else {
      ctx_w->ctx = (sass_context*) cptr;
    }
    ctx_w->request.data = ctx_w;
    ctx_w->callback = new NanCallback(callback);
    ctx_w->errorCallback = new NanCallback(errorCallback);
  }

  if (isFile) {
    sass_file_context* ctx = (sass_file_context*) cptr;
    ctx->input_path = CreateString(options->Get(NanNew("file")));
    ctx->output_path = CreateString(options->Get(NanNew("outFile")));
    ctx->options.image_path = CreateString(options->Get(NanNew("imagePath")));
    ctx->options.output_style = options->Get(NanNew("style"))->Int32Value();
    ctx->options.source_comments = source_comments = options->Get(NanNew("comments"))->Int32Value();
    ctx->omit_source_map_url = options->Get(NanNew("omitSourceMapUrl"))->BooleanValue();
    ctx->options.include_paths = CreateString(options->Get(NanNew("paths")));
    if (source_comments == SASS_SOURCE_COMMENTS_MAP) {
      ctx->source_map_file = CreateString(options->Get(NanNew("sourceMap")));
    }
    ctx->options.precision = options->Get(NanNew("precision"))->Int32Value();
  } else {
    sass_context* ctx = (sass_context*) cptr;
    ctx->source_string = CreateString(options->Get(NanNew("data")));
    ctx->output_path = CreateString(options->Get(NanNew("outFile")));
    ctx->options.image_path = CreateString(options->Get(NanNew("imagePath")));
    ctx->options.output_style = options->Get(NanNew("style"))->Int32Value();
    ctx->options.source_comments = source_comments = options->Get(NanNew("comments"))->Int32Value();
    ctx->omit_source_map_url = options->Get(NanNew("omitSourceMapUrl"))->BooleanValue();
    ctx->options.include_paths = CreateString(options->Get(NanNew("paths")));
    ctx->options.precision = options->Get(NanNew("precision"))->Int32Value();
  }
}

template<typename Ctx>
void FillStatsObj(Handle<Object> stats, Ctx ctx) {
  int i;
  Handle<Array> arr;

  arr = NanNew<Array>(ctx->num_included_files);
  for (i = 0; i < ctx->num_included_files; i++) {
    arr->Set(i, NanNew<String>(ctx->included_files[i]));
  }
  (*stats)->Set(NanNew("includedFiles"), arr);
}

void FillStatsObj(Handle<Object> stats, sass_file_context* ctx) {
  Handle<Value> source_map;

  FillStatsObj<sass_file_context*>(stats, ctx);

  if (ctx->error_status) {
      return;
  }
  if (ctx->options.source_comments == SASS_SOURCE_COMMENTS_MAP) {
    source_map = NanNew<String>(ctx->source_map_string);
  } else {
    source_map = NanNull();
  }
  (*stats)->Set(NanNew("sourceMap"), source_map);
}

void MakeCallback(uv_work_t* req) {
  NanScope();

  TryCatch try_catch;
  sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
  int error_status = ctx_w->ctx ? ctx_w->ctx->error_status : ctx_w->fctx->error_status;

  if (ctx_w->ctx) {
    FillStatsObj(NanNew(ctx_w->stats), ctx_w->ctx);
  } else {
    FillStatsObj(NanNew(ctx_w->stats), ctx_w->fctx);
  }

  if (error_status == 0) {
    // if no error, do callback(null, result)
    char* val = ctx_w->ctx ? ctx_w->ctx->output_string : ctx_w->fctx->output_string;
    Local<Value> argv[] = {
      NanNew<String>(val),
      NanNew(ctx_w->stats)->Get(NanNew("sourceMap"))
    };
    ctx_w->callback->Call(2, argv);
  } else {
    // if error, do callback(error)
    char* err = ctx_w->ctx ? ctx_w->ctx->error_message : ctx_w->fctx->error_message;
    Local<Value> argv[] = {
      NanNew<String>(err),
      NanNew<Integer>(error_status)
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

  FillStatsObj(options->Get(NanNew("stats"))->ToObject(), ctx);

  if (ctx->error_status == 0) {
    Local<String> output = NanNew<String>(ctx->output_string);
    free_context(ctx);
    NanReturnValue(output);
  }

  Local<String> error = NanNew<String>(ctx->error_message);
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

  FillStatsObj(options->Get(NanNew("stats"))->ToObject(), ctx);

  if (ctx->error_status == 0) {
    Local<String> output = NanNew<String>(ctx->output_string);
    free_file_context(ctx);
    NanReturnValue(output);
  }

  Local<String> error = NanNew<String>(ctx->error_message);
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
