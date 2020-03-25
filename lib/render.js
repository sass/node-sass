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

module.exports = function(options, emitter, fin) {
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
    var todo = 1;
    var done = function() {
      if (--todo <= 0) {
        fin();
      }
    };

    if (!destination || stdin) {
      emitter.emit('log', result.css.toString());

      if (sourceMap && !options.sourceMapEmbed) {
        emitter.emit('log', result.map.toString());
      }

      return done();
    }

    emitter.emit('info', chalk.green('Rendering Complete, saving .css file...'));

    mkdirp(path.dirname(destination))
      .then(function(){
        fs.writeFile(destination, result.css.toString(), function(err) {
          if (err) {
            fin(err);
            return;
          }

          emitter.emit('info', chalk.green('Wrote CSS to ' + destination));
          emitter.emit('write', err, destination, result.css.toString());
          done();
        });
      })
      .catch(fin);

    if (sourceMap) {
      todo++;

      mkdirp(path.dirname(sourceMap))
        .then(function(){
          fs.writeFile(sourceMap, result.map, function(err) {
            if (err) {
              fin(err);
              return;
            }

            emitter.emit('info', chalk.green('Wrote Source Map to ' + sourceMap));
            emitter.emit('write-source-map', err, sourceMap, result.map);
            done();
          });
        })
        .catch(fin);
    }

    emitter.emit('render', result.css.toString());
  };

  var error = function(err) {
    fin(JSON.stringify(err, null, 2));
    return;
  };

  var renderCallback = function(err, result) {
    if (err) {
      error(err);
    } else {
      success(result);
    }
  };

  sass.render(renderOptions, renderCallback);
};
