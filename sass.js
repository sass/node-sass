var binding;
var fs = require('fs');
try {
  if (fs.realpathSync(__dirname + '/build')) {
    // use the build version if it exists
    binding = require(__dirname + '/build/Release/binding');
  }
} catch (e) {
  // default to a precompiled binary if no build exists
  var platform_full = process.platform+'-'+process.arch;
  binding = require(__dirname + '/precompiled/'+platform_full+'/binding');
}
if (binding === null) {
  throw new Error('Cannot find appropriate binary library for node-sass');
}
var toString = Object.prototype.toString;
SASS_OUTPUT_STYLE = {
    nested: 0,
    expanded: 1,
    compact: 2,
    compressed: 3
};
exports.render = function(css, callback, options) {
    var paths, style;
    if (toString.call(options) !== '[object Object]') {
        options = {};
    }
    paths = options.include_paths || [];
    if (!((style = options.output_style) in SASS_OUTPUT_STYLE)) {
        style = 'nested';
    }
    return binding.render(css, callback, paths.join(':'), SASS_OUTPUT_STYLE[style]);
};
exports.middleware = require('./lib/middleware');
