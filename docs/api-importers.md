# importer (>= v2.0.0) - _experimental_

**This is an experimental LibSass feature. Use with caution.**

Type: `Function | Function[]` signature `function(url, prev, done)`
Default: `undefined`

Function Parameters and Information:
* `url (String)` - the path in import **as-is**, which [LibSass] encountered
* `prev (String)` - the previously resolved path
* `done (Function)` - a callback function to invoke on async completion, takes an object literal containing
  * `file (String)` - an alternate path for [LibSass] to use **OR**
  * `contents (String)` - the imported contents (for example, read from memory or the file system)

Handles when [LibSass] encounters the `@import` directive. A custom importer allows extension of the [LibSass] engine in both a synchronous and asynchronous manner. In both cases, the goal is to either `return` or call `done()` with an object literal. Depending on the value of the object literal, one of two things will happen.

When returning or calling `done()` with `{ file: "String" }`, the new file path will be assumed for the `@import`. It's recommended to be mindful of the value of `prev` in instances where relative path resolution may be required.

When returning or calling `done()` with `{ contents: "String" }`, the string value will be used as if the file was read in through an external source.

Starting from v3.0.0:

* `this` refers to a contextual scope for the immediate run of `sass.render` or `sass.renderSync`

* importers can return error and LibSass will emit that error in response. For instance:

  ```javascript
  done(new Error('doesn\'t exist!'));
  // or return synchornously
  return new Error('nothing to do here');
  ```

* importer can be an array of functions, which will be called by LibSass in the order of their occurrence in array. This helps user specify special importer for particular kind of path (filesystem, http). If an importer does not want to handle a particular path, it should return `null`. See [functions section](#functions--v300) for more details on Sass types.
