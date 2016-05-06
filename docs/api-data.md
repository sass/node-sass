# data
Type: `String`
Default: `null`
**Special**: `file` or `data` must be specified. In the case that both `file` and `data` options are set, node-sass will give precedence to `data` and use `file` to calculate paths in sourcemaps.

A string to pass to [LibSass] to render. It is recommended that you use `includePaths` in conjunction with this so that [LibSass] can find files when using the `@import` directive.
