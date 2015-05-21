```C
// External import entry
struct Sass_Import {
  char* rel;
  char* abs;
  char* source;
  char* srcmap;
};

// Struct to hold importer callback
struct Sass_C_Import_Descriptor {
  Sass_C_Import_Fn function;
  void*            cookie;
};
```
