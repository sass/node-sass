/**
 * wrap C interfaces of libsass into javascript function.
 * Note: instead of embind with patching libsass, using cwrap to wrap C interface directly.
 * it allows to leave libsass untouched.
 */
const wrapSassInterface = (cwrap) => ({
  /* const char* libsass_version() */
  version: cwrap('libsass_version', 'number'),
  /* Sass_Data_Context* sass_make_data_context (char* source_string) */
  sass_make_data_context: cwrap('sass_make_data_context', 'number', ['number']),
  /*Sass_Context* sass_data_context_get_context (struct Sass_Data_Context* data_ctx) */
  sass_data_context_get_context: cwrap('sass_data_context_get_context', 'number', ['number']),
  /*int sass_compile_data_context (struct Sass_Data_Context* ctx) */
  sass_compile_data_context: cwrap('sass_compile_data_context', 'number', ['number']),
  /*const char* sass_context_get_output_string (struct Sass_Context* ctx) */
  sass_context_get_output_string: cwrap('sass_context_get_output_string', 'number', ['number'])

  //Rest of functions need to be wrapped to create feature parity to existing node-sass
  /*
  '_sass_make_file_context', \
  '_sass_file_context_get_context', \
  '_sass_context_get_options', \
  '_sass_option_set_precision', \
  '_sass_option_set_output_style', \
  '_sass_option_set_source_comments', \
  '_sass_option_set_source_map_embed', \
  '_sass_option_set_source_map_contents', \
  '_sass_option_set_omit_source_map_url', \
  '_sass_option_set_indent', \
  '_sass_option_set_linefeed', \
  '_sass_option_set_input_path', \
  '_sass_option_set_output_path', \
  '_sass_option_set_plugin_path', \
  '_sass_option_set_include_path', \
  '_sass_option_set_source_map_file', \
  '_sass_option_set_source_map_root', \
  '_sass_option_set_c_functions', \
  '_sass_make_importer_list', \
  '_sass_make_importer', \
  '_sass_importer_set_list_entry', \
  '_sass_option_set_c_importers', \
  '_sass_compile_file_context', \
  '_sass_context_get_source_map_string', \
  '_sass_context_get_included_files', \
  '_sass_context_get_error_json', \
  '_sass_context_get_error_message', \
  '_sass_delete_file_context', \
  '_sass_delete_data_context', \
  '_sass_compiler_get_last_import', \
  '_sass_import_get_abs_path', \
  '_sass_compiler_get_context', \
  '_sass_context_get_options', \
  '_sass_make_import_list', \
  '_sass_make_import_entry', \
  '_sass_import_set_error', \*/
});

/**
 * Compile libsass wasm binary, create binding interfaces to compiled runtime.
 */
const loadLibSass = () => {
  //if node.js supports native wasm, load wasm binary. otherwise, fall back to asm.js javascript.
  const runtimeModule = require(`./libsass-${isWasmEnabled() ? 'wasm' : 'asm'}`);

  /**
   *  getModuleLoader returns a factory function to wasm binary.
   *  when factory function is called, it internally compiles wasm binary into asmModule runtime, pass it to callback.
   *  Callback creates javascript bindings to runtime module, includes interops around cwrapped function / pointer operations.
   */
  const moduleLoader = getModuleLoader((asmModule) => {
    const { cwrap, Pointer_stringify, stackSave, stackRestore, stackAlloc, stringToUTF8 } = asmModule;
    console.log(`${isWasmEnabled() ? 'wasm' : 'asm'} runtime successfully loaded`);

    const {
      version,
      sass_make_data_context,
      sass_data_context_get_context,
      sass_compile_data_context,
      sass_context_get_output_string
    } = wrapSassInterface(cwrap);

    const allocString = (value) => {
      const len = (value.length << 2) + 1;
      const ret = stackAlloc(len);
      stringToUTF8(value, ret, len);
      return ret;
    };

    /**
     * Returning javascript module exports, corresponds to node-sass's index.js exports.
     * (libsass_version / renderSync)
     */
    return {
      libsass_version: () => {
        const stack = stackSave();
        const ret = Pointer_stringify(version());
        stackRestore(stack);
        return ret;
      },
      stackSave,
      allocString,
      Pointer_stringify,
      stackRestore,
      sass_make_data_context,
      sass_data_context_get_context,
      sass_compile_data_context,
      sass_context_get_output_string
    };
  }, runtimeModule);

  return moduleLoader();
}