var fs = require('fs'),
    path = require('path');

require('./extensions');

/**
 * Get binding
 *
 * @api private
 */

function getBinding() {
  var candidates = [
    path.join(__dirname, '..', 'build', 'Debug', 'binding.node'),
    path.join(__dirname, '..', 'build', 'Release', 'binding.node'),
    path.join(__dirname, '..', 'vendor', process.sassBinaryName, 'binding.node')
  ];

  var candidate = candidates.filter(fs.existsSync).shift();

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

  if (!outFile || typeof outFile !== 'string' || (!options.data && !file)) {
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
  var stats = {};

  stats.entry = options.file || 'data';
  stats.start = Date.now();

  return stats;
}

/**
 * End stats
 *
 * @param {Object} stats
 * @param {Object} sourceMap
 * @api private
 */

function endStats(stats) {
  stats.end = Date.now();
  stats.duration = stats.end - stats.start;

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
  options.style = getStyle(options) || 0;

  if (options.imagePath && typeof options.imagePath !== 'string') {
    throw new Error('`imagePath` needs to be a string');
  }

  var error = options.error;
  var success = options.success;

  options.error = function(err, code) {
    err = JSON.parse(err);
    err.code = code;

    if (error) {
      error(err);
    }
  };

  options.success = function() {
    var result = options.result;
    var stats = endStats(result.stats);

    if (success) {
      success({
        css: result.css,
        map: result.map,
        stats: stats
      });
    }
  };

  delete options.image_path;
  delete options.include_paths;
  delete options.includePaths;
  delete options.source_comments;
  delete options.sourceComments;

  options.result = {
    stats: getStats(options)
  };

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

  var importer = options.importer;

  if (importer) {
    options.importer = function(file, prev, key) {
      function done(data) {
        console.log(data); // ugly hack
        binding.importedCallback({
          index: key,
          objectLiteral: data
        });
      }

      var result = importer(file, prev, done);

      if (result) {
        done(result);
      }
    };
  }

  options.data ? binding.render(options) : binding.renderFile(options);
};

/**
 * Render sync
 *
 * @param {Object} options
 * @api public
 */

module.exports.renderSync = function(options) {
  options = getOptions(options);

  var importer = options.importer;

  if (importer) {
    options.importer = function(file, prev) {
      return { objectLiteral: importer(file, prev) };
    };
  }

  var status = options.data ? binding.renderSync(options) : binding.renderFileSync(options);
  var result = options.result;

  if(status) {
    result.stats = endStats(result.stats);

    return result;
  }
};

/**
 * API Info
 *
 * @api public
 */

module.exports.info = function() {
  var package = require('../package.json');

  return [
           'node-sass version: ' + package.version,
           'libsass version: ' + package.libsass
         ].join('\n');
};
