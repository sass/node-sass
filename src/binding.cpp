#include <nan.h>
#include "sass_context_wrapper.h"

char* CreateString(Local<Value> value) {
  if(value->IsNull() || !value->IsString()) {
    return const_cast<char*>(""); // return empty string.
  }

  String::Utf8Value string(value);
  char *str = (char *) malloc(string.length() + 1);
  strcpy(str, *string);
  return str;
}

struct Sass_Import** sass_importer(const char* file, void* cookie)
{
  NanScope();

  Handle<Value> argv[] = {
    NanNew<String>(file)
  };

  Local<Value> returned_value = NanNew<Value>(((NanCallback*)cookie)->Call(2, argv));

  if(returned_value->IsArray()) {
    Handle<Array> array = Handle<Array>::Cast(returned_value);

    struct Sass_Import** incs = sass_make_import_list(array->Length());

    for(size_t i = 0; i < array->Length(); ++i) {
      Local<Value> value = array->Get(i);

      if(!value->IsObject())
        continue;

      Local<Object> object = Local<Object>::Cast(value);
      char* path = CreateString(object->Get(String::New("path")));
      char* contents = CreateString(object->Get(String::New("contents")));

      incs[i] = sass_make_import_entry(path, (!contents || contents[0] == '\0') ? 0 : strdup(contents), 0);
    }

    return incs;
  } else if(returned_value->IsObject()) {
    struct Sass_Import** incs = sass_make_import_list(1);
    Local<Object> object = Local<Object>::Cast(returned_value);
    char* path = CreateString(object->Get(String::New("path")));
    char* contents = CreateString(object->Get(String::New("contents")));

    incs[0] = sass_make_import_entry(path, (!contents || contents[0] == '\0') ? 0 : strdup(contents), 0);

    return incs;
  }

  struct Sass_Import** incs = sass_make_import_list(1);

  incs[0] = sass_make_import_entry(file, 0, 0);

  return incs;
}

void ExtractOptions(Local<Object> options, void* cptr, sass_context_wrapper* ctx_w, bool isFile) {
  struct Sass_Context* ctx;

  if (isFile) {
    ctx = sass_file_context_get_context((struct Sass_File_Context*) cptr);
  } else {
    ctx = sass_data_context_get_context((struct Sass_Data_Context*) cptr);
  }

  struct Sass_Options* sass_options = sass_context_get_options(ctx);

  if (ctx_w) {
    NanAssignPersistent(ctx_w->stats, options->Get(NanNew("stats"))->ToObject());

    // async (callback) style
    Local<Function> callback = Local<Function>::Cast(options->Get(NanNew("success")));
    Local<Function> error_callback = Local<Function>::Cast(options->Get(NanNew("error")));
    Local<Function> importer_callback = Local<Function>::Cast(options->Get(NanNew("importer")));

    if (isFile) {
      ctx_w->fctx = (struct Sass_File_Context*) cptr;
    } else {
      ctx_w->dctx = (struct Sass_Data_Context*) cptr;
    }
    ctx_w->request.data = ctx_w;
    ctx_w->callback = new NanCallback(callback);
    ctx_w->error_callback = new NanCallback(error_callback);
    ctx_w->importer_callback = new NanCallback(importer_callback);

    if(!importer_callback->IsUndefined())
      sass_option_set_importer(sass_options, sass_make_importer(sass_importer, ctx_w->importer_callback));
  }

  sass_option_set_output_path(sass_options, CreateString(options->Get(NanNew("outFile"))));
  sass_option_set_image_path(sass_options, CreateString(options->Get(NanNew("imagePath"))));
  sass_option_set_output_style(sass_options, (Sass_Output_Style)options->Get(NanNew("style"))->Int32Value());
  sass_option_set_is_indented_syntax_src(sass_options, options->Get(NanNew("indentedSyntax"))->BooleanValue());
  sass_option_set_source_comments(sass_options, options->Get(NanNew("comments"))->BooleanValue());
  sass_option_set_omit_source_map_url(sass_options, options->Get(NanNew("omitSourceMapUrl"))->BooleanValue());
  sass_option_set_source_map_embed(sass_options, options->Get(NanNew("sourceMapEmbed"))->BooleanValue());
  sass_option_set_source_map_contents(sass_options, options->Get(NanNew("sourceMapContents"))->BooleanValue());
  sass_option_set_source_map_file(sass_options, CreateString(options->Get(NanNew("sourceMap"))));
  sass_option_set_include_path(sass_options, CreateString(options->Get(NanNew("paths"))));
  sass_option_set_precision(sass_options, options->Get(NanNew("precision"))->Int32Value());
}

void FillStatsObj(Handle<Object> stats, Sass_Context* ctx) {
  char** included_files = sass_context_get_included_files(ctx);
  Handle<Array> arr = NanNew<Array>();

  if(included_files) {
    for (int i = 0; included_files[i] != nullptr; ++i) {
      arr->Set(i, NanNew<String>(included_files[i]));
    }
  }

  (*stats)->Set(NanNew("includedFiles"), arr);

  Handle<Value> source_map;

  if (sass_context_get_error_status(ctx)) {
    return;
  }

  if (sass_context_get_source_map_string(ctx)) {
    source_map = NanNew<String>(sass_context_get_source_map_string(ctx));
  } else {
    source_map = NanNew<String>("{}");
  }

  (*stats)->Set(NanNew("sourceMap"), source_map); 
}

void MakeCallback(uv_work_t* req) {
  NanScope();

  TryCatch try_catch;
  sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
  int error_status;
  struct Sass_Context* ctx;

  if (ctx_w->dctx) {
    ctx = sass_data_context_get_context(ctx_w->dctx);
    FillStatsObj(NanNew(ctx_w->stats), ctx);
    error_status = sass_context_get_error_status(ctx);
  } else {
    ctx = sass_file_context_get_context(ctx_w->fctx);
    FillStatsObj(NanNew(ctx_w->stats), ctx);
    error_status = sass_context_get_error_status(ctx);
  }

  if (error_status == 0) {
    // if no error, do callback(null, result)
    const char* val = sass_context_get_output_string(ctx);
    Local<Value> argv[] = {
      NanNew<String>(val),
      NanNew(ctx_w->stats)->Get(NanNew("sourceMap"))
    };
    ctx_w->callback->Call(2, argv);
  } else {
    // if error, do callback(error)
    const char* err = sass_context_get_error_json(ctx);
    Local<Value> argv[] = {
      NanNew<String>(err),
      NanNew<Integer>(error_status)
    };
    ctx_w->error_callback->Call(2, argv);
  }
  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }

  sass_free_context_wrapper(ctx_w);
}

NAN_METHOD(Render) {
  NanScope();

  Local<Object> options = args[0]->ToObject();
  char* source_string = CreateString(options->Get(NanNew("data")));
  struct Sass_Data_Context* dctx = sass_make_data_context(source_string);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper();

  ctx_w->dctx = dctx;

  ExtractOptions(options, dctx, ctx_w, false);

  int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);

  assert(status == 0);

  NanReturnUndefined();
}

NAN_METHOD(RenderSync) {
  NanScope();

  Local<Object> options = args[0]->ToObject();
  char* source_string = CreateString(options->Get(NanNew("data")));
  struct Sass_Data_Context* dctx = sass_make_data_context(source_string);
  struct Sass_Context* ctx = sass_data_context_get_context(dctx);

  ExtractOptions(options, dctx, NULL, false);
  compile_data(dctx);
  FillStatsObj(options->Get(NanNew("stats"))->ToObject(), ctx);

  if (sass_context_get_error_status(ctx) == 0) {
    Local<String> output = NanNew<String>(sass_context_get_output_string(ctx));

    sass_delete_data_context(dctx);
    NanReturnValue(output);
  }

  Local<String> error = NanNew<String>(sass_context_get_error_json(ctx));

  sass_delete_data_context(dctx);
  NanThrowError(error);

  NanReturnUndefined();
}

NAN_METHOD(RenderFile) {
  NanScope();

  Local<Object> options = args[0]->ToObject();
  char* input_path = CreateString(options->Get(NanNew("file")));
  struct Sass_File_Context* fctx = sass_make_file_context(input_path);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper();

  ctx_w->fctx = fctx;
  ExtractOptions(options, fctx, ctx_w, true);

  int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);

  assert(status == 0);
  free(input_path);

  NanReturnUndefined();
}

NAN_METHOD(RenderFileSync) {
  NanScope();

  Local<Object> options = args[0]->ToObject();
  char* input_path = CreateString(options->Get(NanNew("file")));
  struct Sass_File_Context* fctx = sass_make_file_context(input_path);
  struct Sass_Context* ctx = sass_file_context_get_context(fctx);

  ExtractOptions(options, fctx, NULL, true);
  compile_file(fctx);
  FillStatsObj(options->Get(NanNew("stats"))->ToObject(), ctx);
  free(input_path);

  if (sass_context_get_error_status(ctx) == 0) {
    Local<String> output = NanNew<String>(sass_context_get_output_string(ctx));

    sass_delete_file_context(fctx);
    NanReturnValue(output);
  }

  Local<String> error = NanNew<String>(sass_context_get_error_json(ctx));

  sass_delete_file_context(fctx);
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
