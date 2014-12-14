var fs = require('fs'),
    path = require('path');

/**
 * Get binding
 *
 * @api private
 */

function getBinding() {
  var name = process.platform + '-' + process.arch;
  var candidates = [
    path.join(__dirname, '..', 'build', 'Release', 'binding.node'),
    path.join(__dirname, '..', 'build', 'Debug', 'binding.node'),
    path.join(__dirname, '..', 'vendor', name, 'binding.node')
  ];

  var candidate = candidates.filter(fs.existsSync)[0];

  if (!candidate) {
    throw new Error('`libsass` bindings not found. Try reinstalling `node-sass`?');
  }

  return candidate;
}

/**
 * Get outfile
 *
 * @param {Object} options
 * @api private
 */

function getOutFile(options) {
  var file = options.file;
  var outFile = options.outFile;

  if (!file || !outFile || typeof outFile !== 'string' || typeof file !== 'string') {
    return null;
  }

  if (path.resolve(outFile) === path.normalize(outFile).replace(/(.+)([\/|\\])$/, '$1')) {
    return outFile;
  }

  return path.resolve(path.dirname(file), outFile);
}

/**
 * Get stats
 *
 * @param {Object} options
 * @api private
 */

function getStats(options) {
  var stats = options.stats;

  stats.entry = options.file || 'data';
  stats.start = Date.now();

  return stats;
}

/**
 * End stats
 *
 * @param {Object} options
 * @param {Object} sourceMap
 * @api private
 */

function endStats(options, sourceMap) {
  var stats = options.stats || {};

  stats.end = Date.now();
  stats.duration = stats.end - stats.start;
  stats.sourceMap = sourceMap;

  return stats;
}

/**
 * Get style
 *
 * @param {Object} options
 * @api private
 */

function getStyle(options) {
  var style = options.output_style || options.outputStyle;
  var styles = {
    nested: 0,
    expanded: 1,
    compact: 2,
    compressed: 3
  };

  return styles[style];
}

/**
 * Get source map
 *
 * @param {Object} options
 * @api private
 */

function getSourceMap(options) {
  var file = options.file;
  var outFile = options.outFile;
  var sourceMap = options.sourceMap;

  if (sourceMap) {
    if (typeof sourceMap !== 'string') {
      sourceMap = outFile ? outFile + '.map' : '';
    } else if (outFile) {
      sourceMap = path.resolve(path.dirname(file), sourceMap);
    }
  }

  return sourceMap;
}

/**
 * Get options
 *
 * @param {Object} options
 * @api private
 */

function getOptions(options) {
  options = options || {};
  options.comments = options.source_comments || options.sourceComments || false;
  options.data = options.data || null;
  options.file = options.file || null;
  options.imagePath = options.image_path || options.imagePath || '';
  options.outFile = getOutFile(options);
  options.paths = (options.include_paths || options.includePaths || []).join(path.delimiter);
  options.precision = parseInt(options.precision) || 5;
  options.sourceMap = getSourceMap(options);
  options.stats = options.stats || {};
  options.style = getStyle(options) || 0;

  if (options.imagePath && typeof options.imagePath !== 'string') {
    throw new Error('`imagePath` needs to be a string');
  }

  getStats(options);

  var error = options.error;
  var success = options.success;
  var importer = options.importer;

  options.error = function(err, code) {
    try {
      err = JSON.parse(err);
    } catch (e) {
      err = { message: err };
    }

    if (error) {
      error(err, code);
    }
  };

  options.success = function(css, sourceMap) {
    sourceMap = JSON.parse(sourceMap);

    endStats(options, sourceMap);

    if (success) {
      success(css, sourceMap);
    }
  };

  if (importer) {
    options.importer = function(file, prev, key) {
      importer(file, prev, function(data) {
        binding.importedCallback({
          index: key,
          objectLiteral: data
        });
      });
    };
  }

  delete options.image_path;
  delete options.include_paths;
  delete options.includePaths;
  delete options.source_comments;
  delete options.sourceComments;

  return options;
}

/**
 * Require binding
 */

var binding = require(getBinding());

/**
 * Render
 *
 * @param {Object} options
 * @api public
 */

module.exports.render = function(options) {
  options = getOptions(options);
  options.data ? binding.render(options) : binding.renderFile(options);
};

/**
 * Render sync
 *
 * @param {Object} options
 * @api public
 */

module.exports.renderSync = function(options) {
  var output;

  options = getOptions(options);
  output = options.data ? binding.renderSync(options) : binding.renderFileSync(options);

  endStats(options, JSON.parse(options.stats.sourceMap));
  return output;
};

/**
 * Render file
 *
 * `options.sourceMap` can be used to specify that the source map should be saved:
 *
 * - If falsy the source map will not be saved
 * - If `options.sourceMap === true` the source map will be saved to the
 *   standard location of `options.file + '.map'`
 * - Else `options.sourceMap` specifies the path (relative to the `outFile`)
 *   where the source map should be saved
 *
 * @param {Object} options
 * @api public
 */

module.exports.renderFile = function(options) {
  options = options || {};

  var outFile = options.outFile;
  var success = options.success;

  if (options.sourceMap === true) {
    options.sourceMap = outFile + '.map';
  }

  options.success = function(css, sourceMap) {
    fs.writeFile(outFile, css, function(err) {
      if (err) {
        return options.error(err);
      }

      if (!options.sourceMap) {
        return success(outFile);
      }

      var dir = path.dirname(outFile);
      var sourceMapFile = path.resolve(dir, options.sourceMap);

      fs.writeFile(sourceMapFile, JSON.stringify(sourceMap), function(err) {
        if (err) {
          return options.error(err);
        }

        success(outFile, sourceMapFile);
      });
    });
  };

  module.exports.render(options);
};

/**
 * Middleware
 *
 * @api public
 */

module.exports.middleware = function() {
  return new Error([
    'The middleware has been moved to',
    'https://github.com/sass/node-sass-middleware'
  ].join(' '));

  endStats(options, options.stats.sourceMap);

  return {
    css: output,
    map: options.stats.sourceMap
  };
};
