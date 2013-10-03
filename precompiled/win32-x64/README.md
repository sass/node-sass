## Building bindings for Windows

When compiling the bindings for Windows, make sure to remove every instance of the following line from the Visual Studio project file (build/binding.vcxproj)

```xml
<RuntimeTypeInfo>false</RuntimeTypeInfo>
```

This line is known to cause some incompatibility with libsass and node.js.

[More details about the node-gyp issue](https://github.com/TooTallNate/node-gyp/issues/335)