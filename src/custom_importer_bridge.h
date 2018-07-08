#ifndef CUSTOM_IMPORTER_BRIDGE_H
#define CUSTOM_IMPORTER_BRIDGE_H

#include <sass/functions.h>
#include <sass/values.h>
#include "callback_bridge.h"

typedef Sass_Import_List SassImportList;

class CustomImporterBridge : public CallbackBridge<SassImportList> {
  public:
    CustomImporterBridge(napi_env env, napi_value cb, bool is_sync) : CallbackBridge<SassImportList>(env, cb, is_sync) {}

  private:
    SassImportList post_process_return_value(napi_env, napi_value) const;
    Sass_Import* check_returned_string(napi_env, napi_value, const char*) const;
    Sass_Import* get_importer_entry(napi_env, const napi_value&) const;
    std::vector<napi_value> pre_process_args(napi_env, std::vector<void*>) const;
};

#endif
