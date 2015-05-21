```C
// Struct to hold custom function callback
struct Sass_C_Function_Descriptor {
  const char*     signature;
  Sass_C_Function function;
  void*           cookie;
};
```
