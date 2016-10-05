# sourceMap
Type: `Boolean | String | undefined`
Default: `undefined`
**Special:** Setting the `sourceMap` option requires also setting the `outFile` option

Enables the outputting of a source map during `render` and `renderSync`. When `sourceMap === true`, the value of `outFile` is used as the target output location for the source map. When `typeof sourceMap === "string"`, the value of `sourceMap` will be used as the writing location for the file.
