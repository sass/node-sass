#include <stdexcept>
#include "custom_importer_bridge.h"
#include "create_string.h"

SassImportList CustomImporterBridge::post_process_return_value(napi_env env, napi_value returned_value) const {
  SassImportList imports = 0;
  Napi::HandleScope scope(env);

  bool isArray;
  bool isError;
  CHECK_NAPI_RESULT(napi_is_array(env, returned_value, &isArray));
  CHECK_NAPI_RESULT(napi_is_error(env, returned_value, &isError));

  if (isArray) {
    uint32_t length;
    CHECK_NAPI_RESULT(napi_get_array_length(env, returned_value, &length));
    imports = sass_make_import_list(length);

    for (uint32_t i = 0; i < length; ++i) {
      napi_value value;
      CHECK_NAPI_RESULT(napi_get_element(env, returned_value, i, &value));

      napi_valuetype t;
      CHECK_NAPI_RESULT(napi_typeof(env, value, &t));

      if (t != napi_object) {
        auto entry = sass_make_import_entry(0, 0, 0);
        sass_import_set_error(entry, "returned array must only contain object literals", -1, -1);
        continue;
      }

      CHECK_NAPI_RESULT(napi_is_error(env, value, &isError));

      if (isError) {
        napi_value propertyMessage;
        CHECK_NAPI_RESULT(napi_get_named_property(env, value, "message", &propertyMessage));

        char* message = create_string(env, propertyMessage);
        imports[i] = sass_make_import_entry(0, 0, 0);

        sass_import_set_error(imports[i], message, -1, -1);
        free(message);
      }
      else {
        imports[i] = get_importer_entry(env, value);
      }
    }
  }
  else if (isError) {
    imports = sass_make_import_list(1);

    napi_value propertyMessage;
    CHECK_NAPI_RESULT(napi_get_named_property(env, returned_value, "message", &propertyMessage));

    char* message = create_string(env, propertyMessage);
    imports[0] = sass_make_import_entry(0, 0, 0);

    sass_import_set_error(imports[0], message, -1, -1);
    free(message);
  }
  else {
    napi_valuetype t;
    CHECK_NAPI_RESULT(napi_typeof(env, returned_value, &t));

    if (t == napi_object) {
      imports = sass_make_import_list(1);
      imports[0] = get_importer_entry(env, returned_value);
    }
  }

  return imports;
}

Sass_Import* CustomImporterBridge::check_returned_string(napi_env env, napi_value value, const char *msg) const
{
  napi_valuetype t;
  CHECK_NAPI_RESULT(napi_typeof(env, value, &t));

  if (t != napi_undefined && t != napi_string) {
    goto err;
  } else {
    return nullptr;
  }

err:
  auto entry = sass_make_import_entry(0, 0, 0);
  sass_import_set_error(entry, msg, -1, -1);
  return entry;
}

Sass_Import* CustomImporterBridge::get_importer_entry(napi_env env, const napi_value& object) const {
  napi_value returned_file;
  CHECK_NAPI_RESULT(napi_get_named_property(env, object, "file", &returned_file));
  napi_value returned_contents;
  CHECK_NAPI_RESULT(napi_get_named_property(env, object, "contents", &returned_contents));
  napi_value returned_map;
  CHECK_NAPI_RESULT(napi_get_named_property(env, object, "map", &returned_map));

  Sass_Import *err;

  if ((err = check_returned_string(env, returned_file, "returned value of `file` must be a string")))
    return err;

  if ((err = check_returned_string(env, returned_contents, "returned value of `contents` must be a string")))
    return err;

  if ((err = check_returned_string(env, returned_map, "returned value of `returned_map` must be a string")))
    return err;

  char* path = create_string(env, returned_file);
  char* contents = create_string(env, returned_contents);
  char* srcmap = create_string(env, returned_map);

  return sass_make_import_entry(path, contents, srcmap);
}

std::vector<napi_value> CustomImporterBridge::pre_process_args(napi_env env, std::vector<void*> in) const {
  std::vector<napi_value> out;

  for (void* ptr : in) {
    const char* s = (const char*)ptr;
    int len = (int)strlen(s);
    napi_value str;
    CHECK_NAPI_RESULT(napi_create_string_utf8(env, s, len, &str));
    out.push_back(str);
  }

  return out;
}
