#include <vector>
#include "sass_context_wrapper.h"
#include "custom_function_bridge.h"
#include "create_string.h"
#include "sass_types/factory.h"
#include <napi.h>

Sass_Import_List sass_importer(const char* cur_path, Sass_Importer_Entry cb, struct Sass_Compiler* comp)
{
  void* cookie = sass_importer_get_cookie(cb);
  struct Sass_Import* previous = sass_compiler_get_last_import(comp);
  const char* prev_path = sass_import_get_abs_path(previous);
  CustomImporterBridge& bridge = *(static_cast<CustomImporterBridge*>(cookie));

  std::vector<void*> argv;
  argv.push_back((void*)cur_path);
  argv.push_back((void*)prev_path);

  return bridge(argv);
}

union Sass_Value* sass_custom_function(const union Sass_Value* s_args, Sass_Function_Entry cb, struct Sass_Compiler* comp)
{
  void* cookie = sass_function_get_cookie(cb);
  CustomFunctionBridge& bridge = *(static_cast<CustomFunctionBridge*>(cookie));

  std::vector<void*> argv;
  for (size_t l = sass_list_get_length(s_args), i = 0; i < l; i++) {
    argv.push_back((void*)sass_list_get_value(s_args, i));
  }

  return bridge(argv);
}

int ExtractOptions(napi_env e, napi_value options, void* cptr, sass_context_wrapper* ctx_w, bool is_file, bool is_sync) {
  // TODO: Enable napi_close_handle_scope during a pending exception state.
  //Napi::HandleScope scope(e);

  struct Sass_Context* ctx;

  napi_value result_;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "result", &result_));
  napi_valuetype t;
  CHECK_NAPI_RESULT(napi_typeof(e, result_, &t));

  if (t != napi_object) {
    CHECK_NAPI_RESULT(napi_throw_type_error(e, nullptr, "\"result\" element is not an object"));
    return -1;
  }

  CHECK_NAPI_RESULT(napi_create_reference(e, result_, 1, &ctx_w->result));

  if (is_file) {
    ctx_w->fctx = (struct Sass_File_Context*) cptr;
    ctx = sass_file_context_get_context(ctx_w->fctx);
  }
  else {
    ctx_w->dctx = (struct Sass_Data_Context*) cptr;
    ctx = sass_data_context_get_context(ctx_w->dctx);
  }

  struct Sass_Options* sass_options = sass_context_get_options(ctx);

  ctx_w->is_sync = is_sync;

  if (!is_sync) {
    ctx_w->request.data = ctx_w;

    // async (callback) style
    napi_value success_cb;
    CHECK_NAPI_RESULT(napi_get_named_property(e, options, "success", &success_cb));
    napi_valuetype cb_type;
    CHECK_NAPI_RESULT(napi_typeof(e, success_cb, &cb_type));
    if (cb_type == napi_function) {
      CHECK_NAPI_RESULT(napi_create_reference(e, success_cb, 1, &ctx_w->success_callback));
    }

    napi_value error_cb;
    CHECK_NAPI_RESULT(napi_get_named_property(e, options, "error", &error_cb));
    CHECK_NAPI_RESULT(napi_typeof(e, error_cb, &cb_type));
    if (cb_type == napi_function) {
      CHECK_NAPI_RESULT(napi_create_reference(e, error_cb, 1, &ctx_w->error_callback));
    }
  }

  if (!is_file) {
    napi_value propertyFile;
    CHECK_NAPI_RESULT(napi_get_named_property(e, options, "file", &propertyFile));

    ctx_w->file = create_string(e, propertyFile);
    sass_option_set_input_path(sass_options, ctx_w->file);
  }

  napi_value propertyIndentWidth;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "indentWidth", &propertyIndentWidth));
  CHECK_NAPI_RESULT(napi_coerce_to_number(e, propertyIndentWidth, &propertyIndentWidth));
  int32_t indent_width_len;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyIndentWidth, &indent_width_len));

  ctx_w->indent = (char*)malloc(indent_width_len + 1);

  napi_value propertyIndentType;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "indentType", &propertyIndentType));
  CHECK_NAPI_RESULT(napi_coerce_to_number(e, propertyIndentType, &propertyIndentType));
  int32_t indent_type_len;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyIndentType, &indent_type_len));

  strcpy(ctx_w->indent, std::string(
    indent_width_len,
    indent_type_len == 1 ? '\t' : ' '
  ).c_str());

 napi_value propertyLinefeed;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "linefeed", &propertyLinefeed));
  napi_value propertyIncludePaths;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "includePaths", &propertyIncludePaths));
  napi_value propertyOutFile;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "outFile", &propertyOutFile));
  napi_value propertySourceMap;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "sourceMap", &propertySourceMap));
  napi_value propertySourceMapRoot;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "sourceMapRoot", &propertySourceMapRoot));

  ctx_w->linefeed = create_string(e, propertyLinefeed);
  ctx_w->include_path = create_string(e, propertyIncludePaths);
  ctx_w->out_file = create_string(e, propertyOutFile);
  ctx_w->source_map = create_string(e, propertySourceMap);
  ctx_w->source_map_root = create_string(e, propertySourceMapRoot);

  napi_value propertyStyle;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "style", &propertyStyle));
  napi_value propertyIdentedSyntax;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "indentedSyntax", &propertyIdentedSyntax));
  napi_value propertySourceComments;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "sourceComments", &propertySourceComments));
  napi_value propertyOmitSourceMapUrl;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "omitSourceMapUrl", &propertyOmitSourceMapUrl));
  napi_value propertySourceMapEmbed;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "sourceMapEmbed", &propertySourceMapEmbed));
  napi_value propertySourceMapContents;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "sourceMapContents", &propertySourceMapContents));
  napi_value propertyPrecision;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "precision", &propertyPrecision));
  napi_value propertyImporter;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "importer", &propertyImporter));
  napi_value propertyFunctions;
  CHECK_NAPI_RESULT(napi_get_named_property(e, options, "functions", &propertyFunctions));

  int32_t styleVal;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyStyle, &styleVal));
  bool indentedSyntaxVal;
  CHECK_NAPI_RESULT(napi_coerce_to_bool(e, propertyIdentedSyntax, &propertyIdentedSyntax));
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertyIdentedSyntax, &indentedSyntaxVal));
  bool sourceCommentsVal;
  CHECK_NAPI_RESULT(napi_coerce_to_bool(e, propertySourceComments, &propertySourceComments));
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertySourceComments, &sourceCommentsVal));
  bool omitSourceMapUrlVal;
  CHECK_NAPI_RESULT(napi_coerce_to_bool(e, propertyOmitSourceMapUrl, &propertyOmitSourceMapUrl));
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertyOmitSourceMapUrl, &omitSourceMapUrlVal));
  bool sourceMapEmbedVal;
  CHECK_NAPI_RESULT(napi_coerce_to_bool(e, propertySourceMapEmbed, &propertySourceMapEmbed));
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertySourceMapEmbed, &sourceMapEmbedVal));
  bool sourceMapContentsVal;
  CHECK_NAPI_RESULT(napi_coerce_to_bool(e, propertySourceMapContents, &propertySourceMapContents));
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertySourceMapContents, &sourceMapContentsVal));
  int32_t precisionVal;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyPrecision, &precisionVal));

  sass_option_set_output_path(sass_options, ctx_w->out_file);
  sass_option_set_output_style(sass_options, (Sass_Output_Style)styleVal);
  sass_option_set_is_indented_syntax_src(sass_options, indentedSyntaxVal);
  sass_option_set_source_comments(sass_options, sourceCommentsVal);
  sass_option_set_omit_source_map_url(sass_options, omitSourceMapUrlVal);
  sass_option_set_source_map_embed(sass_options, sourceMapEmbedVal);
  sass_option_set_source_map_contents(sass_options, sourceMapContentsVal);
  sass_option_set_source_map_file(sass_options, ctx_w->source_map);
  sass_option_set_source_map_root(sass_options, ctx_w->source_map_root);
  sass_option_set_include_path(sass_options, ctx_w->include_path);
  sass_option_set_precision(sass_options, precisionVal);
  sass_option_set_indent(sass_options, ctx_w->indent);
  sass_option_set_linefeed(sass_options, ctx_w->linefeed);

  CHECK_NAPI_RESULT(napi_typeof(e, propertyImporter, &t));

  if (t == napi_function) {
    CustomImporterBridge *bridge = new CustomImporterBridge(e, propertyImporter, ctx_w->is_sync);
    ctx_w->importer_bridges.push_back(bridge);

    Sass_Importer_List c_importers = sass_make_importer_list(1);
    c_importers[0] = sass_make_importer(sass_importer, 0, bridge);

    sass_option_set_c_importers(sass_options, c_importers);
  }
  else {
    bool isArray;
    CHECK_NAPI_RESULT(napi_is_array(e, propertyImporter, &isArray));

    if (isArray) {
      uint32_t len;
      CHECK_NAPI_RESULT(napi_get_array_length(e, propertyImporter, &len));

      Sass_Importer_List c_importers = sass_make_importer_list(len);

      for (uint32_t i = 0; i < len; ++i) {
        napi_value callback;
        CHECK_NAPI_RESULT(napi_get_element(e, propertyImporter, i, &callback));

        CustomImporterBridge *bridge = new CustomImporterBridge(e, callback, ctx_w->is_sync);
        ctx_w->importer_bridges.push_back(bridge);

        c_importers[i] = sass_make_importer(sass_importer, len - i - 1, bridge);
      }

      sass_option_set_c_importers(sass_options, c_importers);
    }
  }

  CHECK_NAPI_RESULT(napi_typeof(e, propertyFunctions, &t));

  if (t == napi_object) {
    // TODO: this should be napi_get_own_propertynames
    napi_value signatures;
    CHECK_NAPI_RESULT(napi_get_property_names(e, propertyFunctions, &signatures));
    uint32_t num_signatures;
    CHECK_NAPI_RESULT(napi_get_array_length(e, signatures, &num_signatures));

    Sass_Function_List fn_list = sass_make_function_list(num_signatures);

    for (uint32_t i = 0; i < num_signatures; i++) {
      napi_value signature;
      CHECK_NAPI_RESULT(napi_get_element(e, signatures, i, &signature));
      napi_value callback;
      CHECK_NAPI_RESULT(napi_get_property(e, propertyFunctions, signature, &callback));

      CustomFunctionBridge *bridge = new CustomFunctionBridge(e, callback, ctx_w->is_sync);
      ctx_w->function_bridges.push_back(bridge);

      char* sig = create_string(e, signature);
      Sass_Function_Entry fn = sass_make_function(sig, sass_custom_function, bridge);
      free(sig);
      sass_function_set_list_entry(fn_list, i, fn);
    }

    sass_option_set_c_functions(sass_options, fn_list);
  }
  return 0;
}

void GetStats(napi_env env, sass_context_wrapper* ctx_w, Sass_Context* ctx) {
  // TODO: Enable napi_close_handle_scope during a pending exception state.
  //Napi::HandleScope scope;

  char** included_files = sass_context_get_included_files(ctx);
  napi_value arr;
  CHECK_NAPI_RESULT(napi_create_array(env, &arr));

  if (included_files) {
    for (int i = 0; included_files[i] != nullptr; ++i) {
      const char* s = included_files[i];
      int len = (int)strlen(s);
      napi_value str;
      CHECK_NAPI_RESULT(napi_create_string_utf8(env, s, len, &str));
      CHECK_NAPI_RESULT(napi_set_element(env, arr, i, str));
    }
  }

  napi_value result;
  CHECK_NAPI_RESULT(napi_get_reference_value(env, ctx_w->result, &result));
  assert(result != nullptr);

  napi_value propertyStats;
  CHECK_NAPI_RESULT(napi_get_named_property(env, result, "stats", &propertyStats));
  napi_valuetype t;
  CHECK_NAPI_RESULT(napi_typeof(env, propertyStats, &t));

  if (t == napi_object) {
    CHECK_NAPI_RESULT(napi_set_named_property(env, propertyStats, "includedFiles", arr));
  } else {
    CHECK_NAPI_RESULT(napi_throw_type_error(env, nullptr, "\"result.stats\" element is not an object"));
  }
}

int GetResult(napi_env env, sass_context_wrapper* ctx_w, Sass_Context* ctx, bool is_sync = false) {
  // TODO: Enable napi_close_handle_scope during a pending exception state.
  //Napi::HandleScope scope(env);
  int status = sass_context_get_error_status(ctx);

  napi_value result;
  CHECK_NAPI_RESULT(napi_get_reference_value(env, ctx_w->result, &result));
  assert(result != nullptr);

  if (status == 0) {
    const char* css = sass_context_get_output_string(ctx);
    const char* map = sass_context_get_source_map_string(ctx);
    size_t css_len = strlen(css);

    napi_value cssBuffer;
    CHECK_NAPI_RESULT(napi_create_buffer_copy(env, css_len, css, NULL, &cssBuffer));
    CHECK_NAPI_RESULT(napi_set_named_property(env, result, "css", cssBuffer));

    GetStats(env, ctx_w, ctx);

    if (map) {
      size_t map_len = strlen(map);
      napi_value mapBuffer;
      CHECK_NAPI_RESULT(napi_create_buffer_copy(env, map_len, map, NULL, &mapBuffer));
      CHECK_NAPI_RESULT(napi_set_named_property(env, result, "map", mapBuffer));
    }
  }
  else if (is_sync) {
    const char* err = sass_context_get_error_json(ctx);
    size_t err_len = strlen(err);
    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, err, err_len, &str));
    CHECK_NAPI_RESULT(napi_set_named_property(env, result, "error", str));
  }

  return status;
}

// ASYNC
// void PerformCall(sass_context_wrapper* ctx_w, Nan::Callback* callback, int argc, v8::Local<v8::Value> argv[]) {
//   if (ctx_w->is_sync) {
//     Nan::Call(*callback, argc, argv);
//   } else {
//     callback->Call(argc, argv, ctx_w->async_resource);
//   }
// }

void MakeCallback(uv_work_t* req) {
  sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
  struct Sass_Context* ctx;

  Napi::HandleScope scope(ctx_w->env);

  if (ctx_w->dctx) {
    ctx = sass_data_context_get_context(ctx_w->dctx);
  }
  else {
    ctx = sass_file_context_get_context(ctx_w->fctx);
  }

  int status = GetResult(ctx_w->env, ctx_w, ctx);

  napi_value global;
  CHECK_NAPI_RESULT(napi_get_global(ctx_w->env, &global));

  if (status == 0 && ctx_w->success_callback) {
    // if no error, do callback(null, result)
    napi_value success_cb;
    CHECK_NAPI_RESULT(napi_get_reference_value(ctx_w->env, ctx_w->success_callback, &success_cb));
    assert(success_cb != nullptr);

    napi_value unused;
    CHECK_NAPI_RESULT(napi_make_callback(ctx_w->env, nullptr, global, success_cb, 0, nullptr, &unused));
  }
  else if (ctx_w->error_callback) {
    // if error, do callback(error)
    const char* err = sass_context_get_error_json(ctx);
    int len = (int)strlen(err);
    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(ctx_w->env, err, len, &str));

    napi_value argv[] = {
      str
    };

    napi_value error_cb;
    CHECK_NAPI_RESULT(napi_get_reference_value(ctx_w->env, ctx_w->error_callback, &error_cb));
    assert(error_cb != nullptr);

    napi_value unused;
    CHECK_NAPI_RESULT(napi_make_callback(ctx_w->env, nullptr, global, error_cb, 1, argv, &unused));
  }

  bool isPending;
  CHECK_NAPI_RESULT(napi_is_exception_pending(ctx_w->env, &isPending));
  if (isPending) {
    napi_value result;
    CHECK_NAPI_RESULT(napi_get_and_clear_last_exception(ctx_w->env, &result));
    CHECK_NAPI_RESULT(napi_fatal_exception(ctx_w->env, result));
  }

  sass_free_context_wrapper(ctx_w);
}

napi_value render(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, &options, nullptr, nullptr));

  napi_value propertyData;
  CHECK_NAPI_RESULT(napi_get_named_property(env, options, "data", &propertyData));
  char* source_string = create_string(env, propertyData);

  struct Sass_Data_Context* dctx = sass_make_data_context(source_string);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);

  // ASYNC
  // ctx_w->async_resource = new Nan::AsyncResource("node-sass:sass_context_wrapper:render");

  if (ExtractOptions(env, options, dctx, ctx_w, false, false) >= 0) {

    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);

    assert(status == 0);
  }

  napi_value ret;
  CHECK_NAPI_RESULT(napi_get_null(env, &ret));
  return ret;
}

napi_value render_sync(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, &options, nullptr, nullptr));

  napi_value propertyData;
  CHECK_NAPI_RESULT(napi_get_named_property(env, options, "data", &propertyData));
  char* source_string = create_string(env, propertyData);

  struct Sass_Data_Context* dctx = sass_make_data_context(source_string);
  struct Sass_Context* ctx = sass_data_context_get_context(dctx);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);
  int result = -1;

  if ((result = ExtractOptions(env, options, dctx, ctx_w, false, true)) >= 0) {
    compile_data(dctx);
    result = GetResult(env, ctx_w, ctx, true);
  }

  sass_free_context_wrapper(ctx_w);

  bool isExceptionPending;
  CHECK_NAPI_RESULT(napi_is_exception_pending(env, &isExceptionPending));
  if (!isExceptionPending) {
    napi_value boolResult;
    CHECK_NAPI_RESULT(napi_get_boolean(env, result == 0, &boolResult));
    return boolResult;
  }

  return nullptr;
}

napi_value render_file(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, &options, nullptr, nullptr));

  napi_value propertyFile;
  CHECK_NAPI_RESULT(napi_get_named_property(env, options, "file", &propertyFile));
  char* input_path = create_string(env, propertyFile);

  struct Sass_File_Context* fctx = sass_make_file_context(input_path);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);

  // ASYNC
  // ctx_w->async_resource = new Nan::AsyncResource("node-sass:sass_context_wrapper:render_file");

  if (ExtractOptions(env, options, fctx, ctx_w, true, false) >= 0) {

    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);
    assert(status == 0);
  }

  napi_value ret;
  CHECK_NAPI_RESULT(napi_get_null(env, &ret));
  return ret;
}

napi_value render_file_sync(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, &options, nullptr, nullptr));

  napi_value propertyFile;
  CHECK_NAPI_RESULT(napi_get_named_property(env, options, "file", &propertyFile));
  char* input_path = create_string(env, propertyFile);

  struct Sass_File_Context* fctx = sass_make_file_context(input_path);
  struct Sass_Context* ctx = sass_file_context_get_context(fctx);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);
  int result = -1;

  if ((result = ExtractOptions(env, options, fctx, ctx_w, true, true)) >= 0) {
    compile_file(fctx);
    result = GetResult(env, ctx_w, ctx, true);
  };

  free(input_path);
  sass_free_context_wrapper(ctx_w);

  napi_value b;
  CHECK_NAPI_RESULT(napi_get_boolean(env, result == 0, &b));
  return b;
}

napi_value libsass_version(napi_env env, napi_callback_info info) {
  const char* ver = libsass_version();
  int len = (int)strlen(ver);
  napi_value str;

  CHECK_NAPI_RESULT(napi_create_string_utf8(env, ver, len, &str));
  return str;
}

napi_value Init(napi_env env, napi_value target) {
  napi_value functionRender;
  CHECK_NAPI_RESULT(napi_create_function(env, "render", NAPI_AUTO_LENGTH, render, nullptr, &functionRender));
  napi_value functionRenderSync;
  CHECK_NAPI_RESULT(napi_create_function(env, "renderSync", NAPI_AUTO_LENGTH, render_sync, nullptr, &functionRenderSync));
  napi_value functionRenderFile;
  CHECK_NAPI_RESULT(napi_create_function(env, "renderFile", NAPI_AUTO_LENGTH, render_file, nullptr, &functionRenderFile));
  napi_value functionRenderFileSync;
  CHECK_NAPI_RESULT(napi_create_function(env, "renderFileSync", NAPI_AUTO_LENGTH, render_file_sync, nullptr, &functionRenderFileSync));
  napi_value functionLibsassVersion;
  CHECK_NAPI_RESULT(napi_create_function(env, "libsassVersion", NAPI_AUTO_LENGTH, libsass_version, nullptr, &functionLibsassVersion));

  CHECK_NAPI_RESULT(napi_set_named_property(env, target, "render", functionRender));
  CHECK_NAPI_RESULT(napi_set_named_property(env, target, "renderSync", functionRenderSync));
  CHECK_NAPI_RESULT(napi_set_named_property(env, target, "renderFile", functionRenderFile));
  CHECK_NAPI_RESULT(napi_set_named_property(env, target, "renderFileSync", functionRenderFileSync));
  CHECK_NAPI_RESULT(napi_set_named_property(env, target, "libsassVersion", functionLibsassVersion));

  SassTypes::Factory::initExports(env, target);
  return target;
}

NAPI_MODULE(binding, Init)
