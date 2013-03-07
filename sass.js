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

var SASS_OUTPUT_STYLE = {
    nested: 0,
    expanded: 1,
    compact: 2,
    compressed: 3
};

exports.render = function(css, callback, options) {
    var paths, style;
    options = typeof options !== 'object' ? {} : options;
    paths = options.include_paths || options.includePaths || [];
    style = SASS_OUTPUT_STYLE[options.output_style || options.outputStyle] || 0;
    return binding.render(css, callback, paths.join(':'), style);
};

exports.middleware = require('./lib/middleware');
