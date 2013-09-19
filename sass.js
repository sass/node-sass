var binding;
var fs = require('fs');
var path = require('path');

var v8 = 'v8-' + /[0-9]+\.[0-9]+/.exec(process.versions.v8)[0];
var modPath = path.join(__dirname, 'bin', process.platform + '-' + process.arch + '-' + v8, 'binding');
try {
  if (fs.realpathSync(__dirname + '/build')) {
    // use the build version if it exists
    binding = require(__dirname + '/build/Release/binding');
  }
} catch (e) {
  try {
    fs.realpathSync(modPath + '.node');
    binding = require(modPath);
  } catch (ex) {
    // No binary!
    throw new Error('`'+ modPath + '.node` is missing. Try reinstalling `node-sass`?');
  }
}

var SASS_OUTPUT_STYLE = {
    nested: 0,
    expanded: 1,
    compact: 2,
    compressed: 3
};

var SASS_SOURCE_COMMENTS = {
  none: 0,
  // This is called default in libsass, but is a reserved keyword here
  normal: 1,
  map: 2
};

var prepareOptions = function(options) {
  var paths, style;
  var options = typeof options !== 'object' ? {} : options;
  var paths = options.include_paths || options.includePaths || [];
  var style = SASS_OUTPUT_STYLE[options.output_style || options.outputStyle] || 0;
  var comments = SASS_SOURCE_COMMENTS[options.source_comments || options.sourceComments] || 0;

  return {
    paths: paths,
    style: style,
    comments: comments
  };
};

var deprecatedRender = function(css, callback, options) {
  options = prepareOptions(options);
  return binding.oldRender(css, callback, options.paths.join(':'), options.style, options.comments);
};

var deprecatedRenderSync = function(css, options) {
  options = prepareOptions(options);
  return binding.renderSync(css, options.paths.join(':'), options.style, options.comments);
};

exports.render = function(options) {
  var newOptions;

  if (typeof arguments[0] === 'string') {
    return deprecatedRender.apply(this, arguments);
  }

  newOptions = prepareOptions(options);
  options.error = options.error || function(){};

  if (options.file !== undefined && options.file !== null) {
    return binding.renderFile(options.file, options.success, options.error, newOptions.paths.join(path.delimiter), newOptions.style, newOptions.comments);
  }

  //Assume data is present if file is not. binding/libsass will tell the user otherwise!
  return binding.render(options.data, options.success, options.error, newOptions.paths.join(path.delimiter), newOptions.style);
};

exports.renderSync = function(options) {
  var newOptions;

  if (typeof arguments[0] === 'string') {
    return deprecatedRenderSync.apply(this, arguments);
  }

  newOptions = prepareOptions(options);

  if (options.file !== undefined && options.file !== null) {
    return binding.renderFileSync(options.file, newOptions.paths.join(':'), newOptions.style, newOptions.comments);
  }

  //Assume data is present if file is not. binding/libsass will tell the user otherwise!
  return binding.renderSync(options.data, newOptions.paths.join(":"), newOptions.style);
};

exports.middleware = require('./lib/middleware');
