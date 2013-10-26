var sass = require('../sass'),
    chalk = require('chalk'),
    fs = require('fs');

function render(options, emitter) {

  sass.render({
    file: options.inFile,
    includePaths: options.includePaths,
    outputStyle: options.outputStyle,
    sourceComments: options.sourceComments,
    success: function(css) {

      emitter.emit('warn', chalk.green('Rendering Complete, saving .css file...'));

      fs.writeFile(options.outFile, css, function(err) {
        if (err) { return emitter.emit('error', chalk.red('Error: ' + err)); }
        emitter.emit('warn', chalk.green('Wrote CSS to ' + options.outFile));
        emitter.emit('write', err, options.outFile, css);
      });

      if (options.stdout) {
        emitter.emit('log', css);
      }

      emitter.emit('render', css);
    },
    error: function(error) {
      emitter.emit('error', error);
    }
  });
}

module.exports = render;
