var fs = require('fs'),
    chalk = require('chalk'),
    sass = require('./'),
    path = require('path'),
    mkdirp = require('mkdirp');

/**
 * Render
 *
 * @param {Object} options
 * @param {Object} emitter
 * @api public
 */

module.exports = function(options, emitter) {
  var renderOptions = {
    imagePath: options.imagePath,
    includePaths: options.includePath,
    omitSourceMapUrl: options.omitSourceMapUrl,
    indentedSyntax: options.indentedSyntax,
    outFile: options.dest,
    outputStyle: options.outputStyle,
    precision: options.precision,
    sourceComments: options.sourceComments,
    sourceMapEmbed: options.sourceMapEmbed,
    sourceMapContents: options.sourceMapContents,
    sourceMap: options.sourceMap,
    importer: options.importer
  };

  if (options.data) {
    renderOptions.data = options.data;
  } else if (options.src) {
    renderOptions.file = options.src;
  }

  renderOptions.success = function(result) {
    var todo = 1;
    var done = function() {
      if (--todo <= 0) {
        emitter.emit('done');
      }
    };

    if (options.stdout || (!options.dest && !process.stdout.isTTY) || options.stdin) {
      emitter.emit('log', result.css);
      return done();
    }

    emitter.emit('warn', chalk.green('Rendering Complete, saving .css file...'));

    mkdirp(path.dirname(options.dest), function(err) {
      if (err) {
        return emitter.emit('error', chalk.red(err));
      }

      fs.writeFile(options.dest, result.css, function (err) {
        if (err) {
          return emitter.emit('error', chalk.red(err));
        }

        emitter.emit('warn', chalk.green('Wrote CSS to ' + options.dest));
        emitter.emit('write', err, options.dest, result.css);
        done();
      });
    });

    if (options.sourceMap) {
      todo++;

      fs.writeFile(options.sourceMap, result.map, function(err) {
        if (err) {
          return emitter.emit('error', chalk.red('Error' + err));
        }

        emitter.emit('warn', chalk.green('Wrote Source Map to ' + options.sourceMap));
        emitter.emit('write-source-map', err, options.sourceMap, result.sourceMap);
        done();
      });
    }

    emitter.emit('render', result.css);
  };

  renderOptions.error = function(error) {
    emitter.emit('error', chalk.red(JSON.stringify(error, null, 2)));
  };

  sass.render(renderOptions);
};
