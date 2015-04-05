#include <nan.h>
#include "custom_importer_bridge.h"
#include "create_string.h"

SassImportList CustomImporterBridge::post_process_return_value(Handle<Value> val) const {
  SassImportList imports = 0;
  NanScope();

  Local<Value> returned_value = NanNew(val);

  if (returned_value->IsArray()) {
    Handle<Array> array = Handle<Array>::Cast(returned_value);

    imports = sass_make_import_list(array->Length());

    for (size_t i = 0; i < array->Length(); ++i) {
      Local<Value> value = array->Get(static_cast<uint32_t>(i));

      if (!value->IsObject()) {
        continue;
      }

      Local<Object> object = Local<Object>::Cast(value);

      if (value->IsNativeError()) {
        char* message = create_string(object->Get(NanNew<String>("message")));

        imports[i] = sass_make_import_entry(0, 0, 0);

        sass_import_set_error(imports[i], message, -1, -1);
      }
      else {
        char* path = create_string(object->Get(NanNew<String>("file")));
        char* contents = create_string(object->Get(NanNew<String>("contents")));
        char* srcmap = create_string(object->Get(NanNew<String>("map")));

        imports[i] = sass_make_import_entry(path, contents, srcmap);
      }
    }
  }
  else if (returned_value->IsNativeError()) {
    imports = sass_make_import_list(1);
    Local<Object> object = Local<Object>::Cast(returned_value);
    char* message = create_string(object->Get(NanNew<String>("message")));

    imports[0] = sass_make_import_entry(0, 0, 0);

    sass_import_set_error(imports[0], message, -1, -1);
  }
  else if (returned_value->IsObject()) {
    imports = sass_make_import_list(1);
    Local<Object> object = Local<Object>::Cast(returned_value);
    char* path = create_string(object->Get(NanNew<String>("file")));
    char* contents = create_string(object->Get(NanNew<String>("contents")));
    char* srcmap = create_string(object->Get(NanNew<String>("map")));

    imports[0] = sass_make_import_entry(path, contents, srcmap);
  }

  return imports;
}

std::vector<Handle<Value>> CustomImporterBridge::pre_process_args(std::vector<void*> in) const {
  std::vector<Handle<Value>> out;

  for (void* ptr : in) {
    out.push_back(NanNew<String>((char const*)ptr));
  }

  return out;
}
