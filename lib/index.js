/*!
 * node-sass: lib/index.js
 */

var path = require('path'),
    util = require('util');

require('./extensions');

/**
 * Require binding
 */

var binding = require(process.sass.getBinaryPath(true));

/**
 * Get input file
 *
 * @param {Object} options
 * @api private
 */

function getInputFile(options) {
  return options.file ? path.resolve(options.file) : null;
}

/**
 * Get output file
 *
 * @param {Object} options
 * @api private
 */

function getOutputFile(options) {
  var outFile = options.outFile;

  if (!outFile || typeof outFile !== 'string' || (!options.data && !options.file)) {
    return null;
  }

  return path.resolve(outFile);
}

/**
 * Get source map
 *
 * @param {Object} options
 * @api private
 */

function getSourceMap(options) {
  var sourceMap = options.sourceMap;

  if (sourceMap && typeof sourceMap !== 'string' && options.outFile) {
    sourceMap = options.outFile + '.map';
  }

  return sourceMap ? path.resolve(sourceMap) : null;
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
  var style = options.outputStyle;
  var styles = {
    nested: 0,
    expanded: 1,
    compact: 2,
    compressed: 3
  };

  return styles[style];
}

/**
 * Get options
 *
 * @param {Object} options
 * @api private
 */

function getOptions(options, cb) {
  options = options || {};
  options.sourceComments = options.sourceComments || false;
  options.data = options.data || null;
  options.file = getInputFile(options);
  options.outFile = getOutputFile(options);
  options.includePaths = (options.includePaths || []).join(path.delimiter);
  options.precision = parseInt(options.precision) || 5;
  options.sourceMap = getSourceMap(options);
  options.style = getStyle(options) || 0;

  // context object represents node-sass environment
  options.context = { options: options, callback: cb };

  options.result = {
    stats: getStats(options)
  };

  return options;
}

/**
 * Render
 *
 * @param {Object} options
 * @api public
 */

module.exports.render = function(options, cb) {
  options = getOptions(options, cb);

  // options.error and options.success are for libsass binding
  options.error = function(err) {
    var payload = util._extend(new Error(), JSON.parse(err));

    if (cb) {
      options.context.callback.call(options.context, payload, null);
    }
  };

  options.success = function() {
    var result = options.result;
    var stats = endStats(result.stats);
    var payload = {
      css: result.css,
      map: result.map,
      stats: stats
    };

    if (cb) {
      options.context.callback.call(options.context, null, payload);
    }
  };

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

      var result = importer.call(options.context, file, prev, done);

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
      return { objectLiteral: importer.call(options.context, file, prev) };
    };
  }

  var status = options.data ? binding.renderSync(options) : binding.renderFileSync(options);
  var result = options.result;

  if (status) {
    result.stats = endStats(result.stats);
    return result;
  }

  throw util._extend(new Error(), JSON.parse(result.error));
};

/**
 * API Info
 *
 * @api public
 */

module.exports.info = process.sass.versionInfo;
