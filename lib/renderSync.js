var sass = require('../sass'),
    chalk = require('chalk'),
    fs = require('fs'),
    path = require('path');

function renderSync(options, emitter) {

  var css = sass.renderSync({
    file: options.inFile,
    includePaths: options.includePaths,
    imagePath: options.imagePath,
    outputStyle: options.outputStyle,
    sourceComments: options.sourceComments,
    sourceMap: options.sourceMap,
  });

  try {
    fs.mkdirSync(path.dirname(options.outFile));
  } catch(e) { }
  fs.writeFileSync(options.outFile, css);
  emitter.emit('warn', chalk.green('Wrote CSS to ' + options.outFile));
}

module.exports = renderSync;
