# Version information (>= v2.0.0)

Both `node-sass` and `libsass` version info is now exposed via the `info` method:

```javascript
var sass = require('node-sass');

console.log(sass.info);

/*
  it will output something like:

  node-sass       2.0.1   (Wrapper)       [JavaScript]
  libsass         3.1.0   (Sass Compiler) [C/C++]
*/
```

Since node-sass >=v3.0.0 LibSass version is determined at run time.
