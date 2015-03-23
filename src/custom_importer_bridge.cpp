#include <nan.h>
#include <sass_context.h>
#include "custom_importer_bridge.h"
#include "create_string.h"


SassImportList CustomImporterBridge::post_process_return_value(Handle<Value> val) const {
  SassImportList imports;
  NanScope();

  Local<Value> returned_value = NanNew(val);

  if (returned_value->IsArray()) {
    Handle<Array> array = Handle<Array>::Cast(returned_value);

    imports = sass_make_import_list(array->Length());

    for (size_t i = 0; i < array->Length(); ++i) {
      Local<Value> value = array->Get(static_cast<uint32_t>(i));

      if (!value->IsObject())
        continue;

      Local<Object> object = Local<Object>::Cast(value);
      char* path = create_string(object->Get(NanNew<String>("file")));
      char* contents = create_string(object->Get(NanNew<String>("contents")));

      imports[i] = sass_make_import_entry(path, (!contents || contents[0] == '\0') ? 0 : strdup(contents), 0);
    }
  }
  else if (returned_value->IsObject()) {
    imports = sass_make_import_list(1);
    Local<Object> object = Local<Object>::Cast(returned_value);
    char* path = create_string(object->Get(NanNew<String>("file")));
    char* contents = create_string(object->Get(NanNew<String>("contents")));

    imports[0] = sass_make_import_entry(path, (!contents || contents[0] == '\0') ? 0 : strdup(contents), 0);
  }
  else {
    imports = sass_make_import_list(1);
    imports[0] = sass_make_import_entry((char const*) this->argv[0], 0, 0);
  }

  return imports;
}

std::vector<Handle<Value>> CustomImporterBridge::pre_process_args(std::vector<void*> in) const {
  std::vector<Handle<Value>> out;

  for (void* ptr : in) {
    out.push_back(NanNew<String>((char const*) ptr));
  }

  return out;
}
