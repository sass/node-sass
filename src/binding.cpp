#include <vector>
#include <node_api_helpers.h>
#include "sass_context_wrapper.h"
#include "custom_function_bridge.h"
#include "create_string.h"
#include "sass_types/factory.h"

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
  for (unsigned l = sass_list_get_length(s_args), i = 0; i < l; i++) {
    argv.push_back((void*)sass_list_get_value(s_args, i));
  }

  return bridge(argv);
}

int ExtractOptions(napi_env e, napi_value options, void* cptr, sass_context_wrapper* ctx_w, bool is_file, bool is_sync) {
  Napi::HandleScope scope;

  struct Sass_Context* ctx;

  napi_propertyname nameResult;
  CHECK_NAPI_RESULT(napi_property_name(e, "result", &nameResult));
  napi_value result_;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameResult, &result_));
  napi_valuetype t;
  CHECK_NAPI_RESULT(napi_get_type_of_value(e, result_, &t));

  if (t != napi_object) {
    CHECK_NAPI_RESULT(napi_throw_type_error(e, "\"result\" element is not an object"));
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

    napi_propertyname nameSuccess;
    CHECK_NAPI_RESULT(napi_property_name(e, "success", &nameSuccess));
    napi_propertyname nameError;
    CHECK_NAPI_RESULT(napi_property_name(e, "error", &nameError));

    CHECK_NAPI_RESULT(napi_get_property(e, options, nameSuccess, &ctx_w->success_callback));
    CHECK_NAPI_RESULT(napi_get_property(e, options, nameError, &ctx_w->error_callback));
  }

  if (!is_file) {
    napi_propertyname nameFile;
    CHECK_NAPI_RESULT(napi_property_name(e, "file", &nameFile));
    napi_value propertyFile;
    CHECK_NAPI_RESULT(napi_get_property(e, options, nameFile, &propertyFile));

    ctx_w->file = create_string(e, propertyFile);
    sass_option_set_input_path(sass_options, ctx_w->file);
  }

  napi_propertyname nameIndentWidth;
  CHECK_NAPI_RESULT(napi_property_name(e, "indentWidth", &nameIndentWidth));
  napi_value propertyIndentWidth;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameIndentWidth, &propertyIndentWidth));
  int32_t indent_width_len;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyIndentWidth, &indent_width_len));

  ctx_w->indent = (char*)malloc(indent_width_len + 1);

  napi_propertyname nameIndentType;
  CHECK_NAPI_RESULT(napi_property_name(e, "indentType", &nameIndentType));
  napi_value propertyIndentType;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameIndentType, &propertyIndentType));
  int32_t indent_type_len;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyIndentType, &indent_type_len));

  strcpy(ctx_w->indent, std::string(
    indent_width_len,
    indent_type_len == 1 ? '\t' : ' '
  ).c_str());

  napi_propertyname nameLinefeed;
  CHECK_NAPI_RESULT(napi_property_name(e, "linefeed", &nameLinefeed));
  napi_propertyname nameIncludePaths;
  CHECK_NAPI_RESULT(napi_property_name(e, "includePaths", &nameIncludePaths));
  napi_propertyname nameOutFile;
  CHECK_NAPI_RESULT(napi_property_name(e, "outFile", &nameOutFile));
  napi_propertyname nameSourceMap;
  CHECK_NAPI_RESULT(napi_property_name(e, "sourceMap", &nameSourceMap));
  napi_propertyname nameSourceMapRoot;
  CHECK_NAPI_RESULT(napi_property_name(e, "sourceMapRoot", &nameSourceMapRoot));

  napi_value propertyLinefeed;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameLinefeed, &propertyLinefeed));
  napi_value propertyIncludePaths;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameIncludePaths, &propertyIncludePaths));
  napi_value propertyOutFile;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameOutFile, &propertyOutFile));
  napi_value propertySourceMap;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameSourceMap, &propertySourceMap));
  napi_value propertySourceMapRoot;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameSourceMapRoot, &propertySourceMapRoot));

  ctx_w->linefeed = create_string(e, propertyLinefeed);
  ctx_w->include_path = create_string(e, propertyIncludePaths);
  ctx_w->out_file = create_string(e, propertyOutFile);
  ctx_w->source_map = create_string(e, propertySourceMap);
  ctx_w->source_map_root = create_string(e, propertySourceMapRoot);

  napi_propertyname nameStyle;
  CHECK_NAPI_RESULT(napi_property_name(e, "style", &nameStyle));
  napi_propertyname nameIdentedSyntax;
  CHECK_NAPI_RESULT(napi_property_name(e, "indentedSyntax", &nameIdentedSyntax));
  napi_propertyname nameSourceComments;
  CHECK_NAPI_RESULT(napi_property_name(e, "sourceComments", &nameSourceComments));
  napi_propertyname nameOmitSourceMapUrl;
  CHECK_NAPI_RESULT(napi_property_name(e, "omitSourceMapUrl", &nameOmitSourceMapUrl));
  napi_propertyname nameSourceMapEmbed;
  CHECK_NAPI_RESULT(napi_property_name(e, "sourceMapEmbed", &nameSourceMapEmbed));
  napi_propertyname nameSourceMapContents;
  CHECK_NAPI_RESULT(napi_property_name(e, "sourceMapContents", &nameSourceMapContents));
  napi_propertyname namePrecision;
  CHECK_NAPI_RESULT(napi_property_name(e, "precision", &namePrecision));
  napi_propertyname nameImporter;
  CHECK_NAPI_RESULT(napi_property_name(e, "importer", &nameImporter));
  napi_propertyname nameFunctions;
  CHECK_NAPI_RESULT(napi_property_name(e, "functions", &nameFunctions));

  napi_value propertyStyle;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameStyle, &propertyStyle));
  napi_value propertyIdentedSyntax;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameIdentedSyntax, &propertyIdentedSyntax));
  napi_value propertySourceComments;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameSourceComments, &propertySourceComments));
  napi_value propertyOmitSourceMapUrl;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameOmitSourceMapUrl, &propertyOmitSourceMapUrl));
  napi_value propertySourceMapEmbed;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameSourceMapEmbed, &propertySourceMapEmbed));
  napi_value propertySourceMapContents;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameSourceMapContents, &propertySourceMapContents));
  napi_value propertyPrecision;
  CHECK_NAPI_RESULT(napi_get_property(e, options, namePrecision, &propertyPrecision));
  napi_value propertyImporter;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameImporter, &propertyImporter));
  napi_value propertyFunctions;
  CHECK_NAPI_RESULT(napi_get_property(e, options, nameFunctions, &propertyFunctions));

  int32_t styleVal;
  CHECK_NAPI_RESULT(napi_get_value_int32(e, propertyStyle, &styleVal));
  bool indentedSyntaxVal;
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertyIdentedSyntax, &indentedSyntaxVal));
  bool sourceCommentsVal;
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertySourceComments, &sourceCommentsVal));
  bool omitSourceMapUrlVal;
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertyOmitSourceMapUrl, &omitSourceMapUrlVal));
  bool sourceMapEmbedVal;
  CHECK_NAPI_RESULT(napi_get_value_bool(e, propertySourceMapEmbed, &sourceMapEmbedVal));
  bool sourceMapContentsVal;
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

  CHECK_NAPI_RESULT(napi_get_type_of_value(e, propertyImporter, &t));

  if (t == napi_function) {
    CustomImporterBridge *bridge = new CustomImporterBridge(e, propertyImporter, ctx_w->is_sync);
    ctx_w->importer_bridges.push_back(bridge);

    Sass_Importer_List c_importers = sass_make_importer_list(1);
    c_importers[0] = sass_make_importer(sass_importer, 0, bridge);

    sass_option_set_c_importers(sass_options, c_importers);
  } else {
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

  CHECK_NAPI_RESULT(napi_get_type_of_value(e, propertyFunctions, &t));

  if (t == napi_object) {
    // TODO: this should be napi_get_own_propertynames
    napi_value signatures;
    CHECK_NAPI_RESULT(napi_get_propertynames(e, propertyFunctions, &signatures));
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

      Sass_Function_Entry fn = sass_make_function(create_string(e, signature), sass_custom_function, bridge);
      sass_function_set_list_entry(fn_list, i, fn);
    }

    sass_option_set_c_functions(sass_options, fn_list);
  }
  return 0;
}

void GetStats(napi_env env, sass_context_wrapper* ctx_w, Sass_Context* ctx) {
  Napi::HandleScope scope;

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
  napi_propertyname nameStats;
  CHECK_NAPI_RESULT(napi_property_name(env, "stats", &nameStats));
  napi_value propertyStats;
  CHECK_NAPI_RESULT(napi_get_property(env, result, nameStats, &propertyStats));
  napi_valuetype t;
  CHECK_NAPI_RESULT(napi_get_type_of_value(env, propertyStats, &t));

  if (t == napi_object) {
    napi_propertyname nameIncludedFiles;
    CHECK_NAPI_RESULT(napi_property_name(env, "includedFiles", &nameIncludedFiles));
    CHECK_NAPI_RESULT(napi_set_property(env, propertyStats, nameIncludedFiles, arr));
  } else {
    CHECK_NAPI_RESULT(napi_throw_type_error(env, "\"result.stats\" element is not an object"));
  }
}

int GetResult(napi_env env, sass_context_wrapper* ctx_w, Sass_Context* ctx, bool is_sync = false) {
  Napi::HandleScope scope;

  int status = sass_context_get_error_status(ctx);
  napi_value result;
  CHECK_NAPI_RESULT(napi_get_reference_value(env, ctx_w->result, &result));

  if (status == 0) {
    const char* css = sass_context_get_output_string(ctx);
    int css_len = (int)strlen(css);
    const char* map = sass_context_get_source_map_string(ctx);

    napi_propertyname nameCss;
    CHECK_NAPI_RESULT(napi_property_name(env, "css", &nameCss));
    napi_value cssBuffer;
    CHECK_NAPI_RESULT(napi_create_buffer_copy(env, css, css_len, &cssBuffer));
    CHECK_NAPI_RESULT(napi_set_property(env, result, nameCss, cssBuffer));

    GetStats(env, ctx_w, ctx);

    if (map) {
      int map_len = (int)strlen(map);
      napi_propertyname nameMap;
      CHECK_NAPI_RESULT(napi_property_name(env, "map", &nameMap));
      napi_value mapBuffer;
      CHECK_NAPI_RESULT(napi_create_buffer_copy(env, map, map_len, &mapBuffer));
      CHECK_NAPI_RESULT(napi_set_property(env, result, nameMap, mapBuffer));
    }
  } else if (is_sync) {
    const char* err = sass_context_get_error_json(ctx);
    int err_len = (int)strlen(err);
    napi_propertyname nameError;
    CHECK_NAPI_RESULT(napi_property_name(env, "error", &nameError));
    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, err, err_len, &str));
    CHECK_NAPI_RESULT(napi_set_property(env, result, nameError, str));
  }

  return status;
}

void MakeCallback(uv_work_t* req) {
  Napi::HandleScope scope;
  napi_env env;
  CHECK_NAPI_RESULT(napi_get_current_env(&env));

  sass_context_wrapper* ctx_w = static_cast<sass_context_wrapper*>(req->data);
  struct Sass_Context* ctx;

  if (ctx_w->dctx) {
    ctx = sass_data_context_get_context(ctx_w->dctx);
  }
  else {
    ctx = sass_file_context_get_context(ctx_w->fctx);
  }

  int status = GetResult(env, ctx_w, ctx);

  if (status == 0 && ctx_w->success_callback) {
    // if no error, do callback(null, result)
    napi_value unused;
    CHECK_NAPI_RESULT(napi_make_callback(env, ctx_w->success_callback, ctx_w->success_callback, 0, nullptr, &unused));
  }
  else if (ctx_w->error_callback) {
    // if error, do callback(error)
    const char* err = sass_context_get_error_json(ctx);
    int len = (int)strlen(err);
    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, err, len, &str));

    napi_value argv[] = {
      str
    };

    napi_value unused;
    CHECK_NAPI_RESULT(napi_make_callback(env, ctx_w->error_callback, ctx_w->error_callback, 1, argv, &unused));
  }

  bool r;
  CHECK_NAPI_RESULT(napi_is_exception_pending(env, &r));
  if (r) {
    // TODO: FatalException
    return;
  }

  sass_free_context_wrapper(ctx_w);
}

NAPI_METHOD(render) {
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &options, 1));

  napi_propertyname nameData;
  CHECK_NAPI_RESULT(napi_property_name(env, "data", &nameData));

  napi_value propertyData;
  CHECK_NAPI_RESULT(napi_get_property(env, options, nameData, &propertyData));
  char* source_string = create_string(env, propertyData);

  struct Sass_Data_Context* dctx = sass_make_data_context(source_string);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);

  if (ExtractOptions(env, options, dctx, ctx_w, false, false) >= 0) {
    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);

    assert(status == 0);
  }
}

NAPI_METHOD(render_sync) {
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &options, 1));

  napi_propertyname nameData;
  CHECK_NAPI_RESULT(napi_property_name(env, "data", &nameData));

  napi_value propertyData;
  CHECK_NAPI_RESULT(napi_get_property(env, options, nameData, &propertyData));
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

  napi_value boolResult;
  CHECK_NAPI_RESULT(napi_create_boolean(env, result == 0, &boolResult));
  CHECK_NAPI_RESULT(napi_set_return_value(env, info, boolResult));
}

NAPI_METHOD(render_file) {
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &options, 1));
  napi_propertyname nameFile;
  CHECK_NAPI_RESULT(napi_property_name(env, "file", &nameFile));
  napi_value propertyFile;
  CHECK_NAPI_RESULT(napi_get_property(env, options, nameFile, &propertyFile));
  char* input_path = create_string(env, propertyFile);

  struct Sass_File_Context* fctx = sass_make_file_context(input_path);
  sass_context_wrapper* ctx_w = sass_make_context_wrapper(env);

  if (ExtractOptions(env, options, fctx, ctx_w, true, false) >= 0) {
    int status = uv_queue_work(uv_default_loop(), &ctx_w->request, compile_it, (uv_after_work_cb)MakeCallback);
    assert(status == 0);
  }
}

NAPI_METHOD(render_file_sync) {
  napi_value options;
  CHECK_NAPI_RESULT(napi_get_cb_args(env, info, &options, 1));
  napi_propertyname nameFile;
  CHECK_NAPI_RESULT(napi_property_name(env, "file", &nameFile));
  napi_value propertyFile;
  CHECK_NAPI_RESULT(napi_get_property(env, options, nameFile, &propertyFile));
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
  CHECK_NAPI_RESULT(napi_create_boolean(env, result == 0, &b));
  CHECK_NAPI_RESULT(napi_set_return_value(env, info, b));
}

NAPI_METHOD(libsass_version) {
  const char* ver = libsass_version();
  int len = (int)strlen(ver);
  napi_value str;
  
  CHECK_NAPI_RESULT(napi_create_string_utf8(env, ver, len, &str));
  CHECK_NAPI_RESULT(napi_set_return_value(env, info, str));
}

void Init(napi_env env, napi_value target, napi_value module) {
  napi_propertyname nameRender;
  CHECK_NAPI_RESULT(napi_property_name(env, "render", &nameRender));
  napi_propertyname nameRenderSync;
  CHECK_NAPI_RESULT(napi_property_name(env, "renderSync", &nameRenderSync));
  napi_propertyname nameRenderFile;
  CHECK_NAPI_RESULT(napi_property_name(env, "renderFile", &nameRenderFile));
  napi_propertyname nameRenderFileSync;
  CHECK_NAPI_RESULT(napi_property_name(env, "renderFileSync", &nameRenderFileSync));
  napi_propertyname nameLibsassVersion;
  CHECK_NAPI_RESULT(napi_property_name(env, "libsassVersion", &nameLibsassVersion));

  napi_value functionRender;
  CHECK_NAPI_RESULT(napi_create_function(env, render, nullptr, &functionRender));
  napi_value functionRenderSync;
  CHECK_NAPI_RESULT(napi_create_function(env, render_sync, nullptr, &functionRenderSync));
  napi_value functionRenderFile;
  CHECK_NAPI_RESULT(napi_create_function(env, render_file, nullptr, &functionRenderFile));
  napi_value functionRenderFileSync;
  CHECK_NAPI_RESULT(napi_create_function(env, render_file_sync, nullptr, &functionRenderFileSync));
  napi_value functionLibsassVersion;
  CHECK_NAPI_RESULT(napi_create_function(env, libsass_version, nullptr, &functionLibsassVersion));

  CHECK_NAPI_RESULT(napi_set_property(env, target, nameRender, functionRender));
  CHECK_NAPI_RESULT(napi_set_property(env, target, nameRenderSync, functionRenderSync));
  CHECK_NAPI_RESULT(napi_set_property(env, target, nameRenderFile, functionRenderFile));
  CHECK_NAPI_RESULT(napi_set_property(env, target, nameRenderFileSync, functionRenderFileSync));
  CHECK_NAPI_RESULT(napi_set_property(env, target, nameLibsassVersion, functionLibsassVersion));

  SassTypes::Factory::initExports(env, target);
}

NODE_MODULE_ABI(binding, Init)
