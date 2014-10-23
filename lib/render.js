var sass = require('../sass'),
    chalk = require('chalk'),
    fs = require('fs');

function render(options, emitter) {
  var renderOptions = {
    imagePath: options.imagePath,
    includePaths: options.includePath,
    omitSourceMapUrl: options.omitSourceMapUrl,
    indentedSyntax: options.indentedSyntax,
    outFile: options.dest,
    outputStyle: options.outputStyle,
    precision: options.precision,
    sourceComments: options.sourceComments,
    sourceMap: options.sourceMap
  };

  if (options.src) {
    renderOptions.file = options.src;
  } else if (options.data) {
    renderOptions.data = options.data;
  }

  renderOptions.success = function(css, sourceMap) {
    var todo = 1;
    var done = function() {
      if (--todo <= 0) {
        emitter.emit('done');
      }
    };

    if (options.stdout || (!options.dest && (!process.stdout.isTTY || !process.env.isTTY))) {
      emitter.emit('log', css);
      return done();
    }

    emitter.emit('warn', chalk.green('Rendering Complete, saving .css file...'));

    fs.writeFile(options.dest, css, function(err) {
      if (err) { return emitter.emit('error', chalk.red('Error: ' + err)); }
      emitter.emit('warn', chalk.green('Wrote CSS to ' + options.dest));
      emitter.emit('write', err, options.dest, css);
      done();
    });

    if (options.sourceMap) {
      todo++;
      fs.writeFile(options.sourceMap, sourceMap, function(err) {
        if (err) {return emitter.emit('error', chalk.red('Error' + err)); }
        emitter.emit('warn', chalk.green('Wrote Source Map to ' + options.sourceMap));
        emitter.emit('write-source-map', err, options.sourceMap, sourceMap);
        done();
      });
    }

    emitter.emit('render', css);
  };

  renderOptions.error = function(error) {
    emitter.emit('error', chalk.red(error));
  };

  sass.render(renderOptions);
}

module.exports = render;
