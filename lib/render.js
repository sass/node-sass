
var sass = require('../sass'),
    colors = require('colors'),
    fs = require('fs');

var cwd = process.cwd();

function render(options) {

  sass.render({
    file: options.inFile,
    includePaths: options.includePaths,
    outputStyle: options.outputStyle,
    sourceComments: options.sourceComments,
    success: function(css) {

      console.warn('Rendering Complete, saving .css file...'.green);

      fs.writeFile(options.outFile, css, function(err) {
        if (err) return console.error(('Error: ' + err).red);
        console.warn(('Wrote CSS to ' + options.outFile).green);
      });

      if (options.stdout) {
        console.log(css);
      }
    },
    error: function(error) {
      console.error(error);
    }
  });
}

module.exports = render;
