# `render` Callback (>= v3.0.0)
node-sass supports standard node style asynchronous callbacks with the signature of `function(err, result)`. In error conditions, the `error` argument is populated with the error object. In success conditions, the `result` object is populated with an object describing the result of the render call.

## Error Object
* `message` (String) - The error message.
* `line` (Number) - The line number of error.
* `column` (Number) - The column number of error.
* `status` (Number) - The status code.
* `file` (String) - The filename of error. In case `file` option was not set (in favour of `data`), this will reflect the value `stdin`.

## Result Object
* `css` (Buffer) - The compiled CSS. Write this to a file, or serve it out as needed.
* `map` (Buffer) - The source map
* `stats` (Object) - An object containing information about the compile. It contains the following keys:
  * `entry` (String) - The path to the scss file, or `data` if the source was not a file
  * `start` (Number) - Date.now() before the compilation
  * `end` (Number) - Date.now() after the compilation
  * `duration` (Number) - *end* - *start*
  * `includedFiles` (Array) - Absolute paths to all related scss files in no particular order.

## Examples

```javascript
var sass = require('node-sass');
sass.render({
  file: '/path/to/myFile.scss',
  data: 'body{background:blue; a{color:black;}}',
  importer: function(url, prev, done) {
    // url is the path in import as is, which LibSass encountered.
    // prev is the previously resolved path.
    // done is an optional callback, either consume it or return value synchronously.
    // this.options contains this options hash, this.callback contains the node-style callback
    someAsyncFunction(url, prev, function(result){
      done({
        file: result.path, // only one of them is required, see section Special Behaviours.
        contents: result.data
      });
    });
    // OR
    var result = someSyncFunction(url, prev);
    return {file: result.path, contents: result.data};
  },
  includePaths: [ 'lib/', 'mod/' ],
  outputStyle: 'compressed'
}, function(error, result) { // node-style callback from v3.0.0 onwards
  if (error) {
    console.log(error.status); // used to be "code" in v2x and below
    console.log(error.column);
    console.log(error.message);
    console.log(error.line);
  }
  else {
    console.log(result.css.toString());

    console.log(result.stats);

    console.log(result.map.toString());
    // or better
    console.log(JSON.stringify(result.map)); // note, JSON.stringify accepts Buffer too
  }
});
// OR
var result = sass.renderSync({
  file: '/path/to/file.scss',
  data: 'body{background:blue; a{color:black;}}',
  outputStyle: 'compressed',
  outFile: '/to/my/output.css',
  sourceMap: true, // or an absolute or relative (to outFile) path
  importer: function(url, prev, done) {
    // url is the path in import as is, which LibSass encountered.
    // prev is the previously resolved path.
    // done is an optional callback, either consume it or return value synchronously.
    // this.options contains this options hash
    someAsyncFunction(url, prev, function(result){
      done({
        file: result.path, // only one of them is required, see section Sepcial Behaviours.
        contents: result.data
      });
    });
    // OR
    var result = someSyncFunction(url, prev);
    return {file: result.path, contents: result.data};
  }
}));

console.log(result.css);
console.log(result.map);
console.log(result.stats);
```
