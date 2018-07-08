/*!
 * node-sass: lib/render.js
 */

var chalk = require('chalk'),
  fs = require('fs'),
  mkdirp = require('mkdirp'),
  path = require('path'),
  sass = require('./');

/**
 * Render
 *
 * @param {Object} options
 * @param {Object} emitter
 * @api public
 */

module.exports = function(options, emitter) {
  var renderOptions = {
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
    sourceMapRoot: options.sourceMapRoot,
    importer: options.importer,
    functions: options.functions,
    indentWidth: options.indentWidth,
    indentType: options.indentType,
    linefeed: options.linefeed
  };

  if (options.data) {
    renderOptions.data = options.data;
  } else if (options.src) {
    renderOptions.file = options.src;
  }

  var sourceMap = options.sourceMap;
  var destination = options.dest;
  var stdin = options.stdin;

  var success = function(result) {
    if (!destination || stdin) {
      emitter.emit('log', result.css.toString());

      if (sourceMap && !options.sourceMapEmbed) {
        emitter.emit('log', result.map.toString());
      }
    } else {
      emitter.emit('info', chalk.green('Rendering Complete, saving .css file...'));

      try {
        mkdirp.sync(path.dirname(destination));
        fs.writeFileSync(destination, result.css.toString());
        emitter.emit('info', chalk.green('Wrote CSS to ' + destination));
        emitter.emit('write', null, destination, result.css.toString());
      } catch (e) {
        return emitter.emit('error', chalk.red(e));
      }

      if (sourceMap) {
        try {
          mkdirp.sync(path.dirname(sourceMap));
          fs.writeFileSync(sourceMap, result.map);
          emitter.emit('info', chalk.green('Wrote Source Map to ' + sourceMap));
          emitter.emit('write-source-map', null, sourceMap, result.map);
        } catch (e) {
          return emitter.emit('error', chalk.red(e));
        }
      }
    }

    emitter.emit('done');
    emitter.emit('render', result.css.toString());
  };

  var error = function(error) {
    emitter.emit('error', chalk.red(JSON.stringify(error, null, 2)));
  };

  try {
    var result = sass.renderSync(renderOptions);
    success(result);
  } catch(e) {
    error(e);
  }
};
