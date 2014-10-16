var path = require('path');
var fs = require('fs');
var assign = require('object-assign');

function requireBinding() {
  var v8 = 'v8-' + /[0-9]+\.[0-9]+/.exec(process.versions.v8)[0];
  var candidates = [
    [__dirname, 'build', 'Release', 'binding.node'],
    [__dirname, 'build', 'Debug', 'binding.node'],
    [__dirname, 'bin', process.platform + '-' + process.arch + '-' + v8, 'binding.node']
  ];
  var candidate;

  for (var i = 0, l = candidates.length; i < l; i++) {
    candidate = path.join.apply(path.join, candidates[i]);

    if (fs.existsSync(candidate)) {
      return require(candidate);
    }
  }

  throw new Error('`libsass` bindings not found. Try reinstalling `node-sass`?');
}

var binding = requireBinding();

var SASS_OUTPUT_STYLE = {
  nested: 0,
  expanded: 1,
  compact: 2,
  compressed: 3
};

var SASS_SOURCE_COMMENTS = {
  none: false,
  normal: true,
  default: false,
  map: true
};

var prepareOptions = function (options) {
  var success;
  var error;
  var stats;
  var sourceComments;
  var sourceMap;

  options = options || {};
  success = options.success;
  error = options.error;
  stats = options.stats || {};

  sourceComments = options.source_comments || options.sourceComments;

  if (options.sourceMap && !sourceComments) {
    sourceComments = 'map';
  }

  if (typeof options.outFile === 'string' && typeof options.file === 'string' && path.resolve(options.outFile) === path.normalize(options.outFile).replace(new RegExp(path.sep + '$'), '' )) {
    options.outFile = path.resolve(path.dirname(options.file), options.outFile);
  }

  sourceMap = options.sourceMap;

  if ((typeof sourceMap !== 'string' || !sourceMap.trim()) && sourceComments === 'map') {
    sourceMap = options.outFile !== null ? options.outFile + '.map' : '';
  } else if (options.outFile && sourceMap) {
    sourceMap = path.resolve(path.dirname(options.file), sourceMap);
  }

  prepareStats(options, stats);

  if (options.imagePath && typeof options.imagePath !== 'string') {
    throw new Error('imagePath needs to be a string');
  }

  return {
    file: options.file || null,
    outFile: options.outFile || null,
    data: options.data || null,
    paths: (options.include_paths || options.includePaths || []).join(path.delimiter),
    imagePath: options.image_path || options.imagePath || '',
    style: SASS_OUTPUT_STYLE[options.output_style || options.outputStyle] || 0,
    comments: SASS_SOURCE_COMMENTS[sourceComments] || false,
    omitSourceMapUrl: options.omitSourceMapUrl,
    indentedSyntax: options.indentedSyntax,
    stats: stats,
    sourceMap: sourceMap,
    precision: parseInt(options.precision) || 5,
    success: function onSuccess(css, sourceMap) {
      finishStats(stats, sourceMap);
      success && success(css, sourceMap);
    },
    error: function onError(err, errStatus) {
      error && error(err, errStatus);
    }
  };
};

var prepareStats = function (options, stats) {
  stats.entry = options.file || 'data';
  stats.start = Date.now();

  return stats;
};

var finishStats = function (stats, sourceMap) {
  stats.end = Date.now();
  stats.duration = stats.end - stats.start;
  stats.sourceMap = sourceMap;

  return stats;
};

var deprecatedRender = function(css, callback, options) {
  options = prepareOptions(options);
  // providing the deprecated single callback signature
  options.error = callback;
  options.success = function(css) {
    callback(null, css);
  };
  options.data = css;
  binding.render(options);
};

var deprecatedRenderSync = function(css, options) {
  options = prepareOptions(options);
  options.data = css;
  return binding.renderSync(options);
};

exports.render = function(options) {
  if (typeof arguments[0] === 'string') {
    return deprecatedRender.apply(this, arguments);
  }

  options = assign({}, options);
  options = prepareOptions(options);
  options.file? binding.renderFile(options) : binding.render(options);
};

exports.renderSync = function(options) {
  var output;

  if (typeof arguments[0] === 'string') {
    return deprecatedRenderSync.apply(this, arguments);
  }

  options = assign({}, options);
  options = prepareOptions(options);
  output = options.file? binding.renderFileSync(options) : binding.renderSync(options);
  finishStats(options.stats);

  return output;
};

/**
  Same as `render()` but with an extra `outFile` property in `options` and writes
  the CSS and sourceMap (if requested) to the filesystem.

  `options.sourceMap` can be used to specify that the source map should be saved:

  - If falsy the source map will not be saved
  - If `options.sourceMap === true` the source map will be saved to the
  standard location of `options.file + '.map'`
  - Else `options.sourceMap` specifies the path (relative to the `outFile`)
  where the source map should be saved
 */
exports.renderFile = function(options) {
  var success;

  options = assign({}, options);
  success = options.success;
  if (options.sourceMap === true) {
    options.sourceMap = options.outFile + '.map';
  }
  options.success = function(css, sourceMap) {
    fs.writeFile(options.outFile, css, function(err) {
      var dir, sourceMapFile;

      if (err) {
        return options.error(err);
      }
      if (options.sourceMap) {
        dir = path.dirname(options.outFile);
        sourceMapFile = path.resolve(dir, options.sourceMap);
        fs.writeFile(sourceMapFile, sourceMap, function(err) {
          if (err) {
            return options.error(err);
          }
          success(options.outFile, sourceMapFile);
        });
      }
      else {
        success(options.outFile);
      }
    });
  };
  exports.render(options);
};

exports.middleware = function() {
  return new Error('The middleware has been moved to https://github.com/sass/node-sass-middleware');
};
