var watch = require('node-watch'),
    render = require('./render'),
    renderSync = require('./renderSync'),
    fs = require('fs'),
    path = require('path'),
    Emitter = require('events').EventEmitter,
    cwd = process.cwd();

var optimist = require('optimist')
  .usage('Compile .scss files with node-sass.\nUsage: $0 [options] <input.scss> [<output.css>]')
  .options('output-style', {
    describe: 'CSS output style (nested|expanded|compact|compressed)',
    'default': 'nested'
  })
  .options('source-comments', {
    describe: 'Include debug info in output (none|normal|map)',
    'default': 'none'
  })
  .options('source-map', {
    describe: 'Emit source map'
  })
  .options('include-path', {
    describe: 'Path to look for @import-ed files',
    'default': cwd
  })
  .options('image-path', {
    describe: 'Path to prepend when using the image-url(…) helper',
    'default': ''
  })
  .options('watch', {
    describe: 'Watch a directory or file',
    alias: 'w'
  })
  .options('outdir', {
    describe: 'Output directory',
    alias: 'd',
    'default': cwd
  })
  .options('output', {
    describe: 'Output css file',
    alias: 'o'
  })
  .options('stdout', {
    describe: 'Print the resulting CSS to stdout'
  })
  .options('help', {
    describe: 'Print usage info',
    type: 'string',
    alias: 'help'
  })
  .check(function(argv){
    if (argv.help) { return true; }
    if (!argv.w && argv._.length < 1) { return false; }
  });

exports = module.exports = function(args) {
  var argv = optimist.parse(args);

  if (argv.help) {
    optimist.showHelp();
    process.exit(0);
    return;
  }

  var emitter = new Emitter();

  emitter.on('error', function(err){
    console.error(err);
    process.exit(1);
  });

  var options = {
    stdout: argv.stdout
  };

  var inFile = options.inFile = argv._[0];
  var outFile = options.outFile = argv.o || argv._[1];
  var outDir = fs.lstatSync(argv.d).isDirectory() ? argv.d : cwd;

  if (!outFile) {
    var suffix = '.css';
    if (/\.css$/.test(inFile)) {
      suffix = '';
    }
    outFile = options.outFile = path.join(outDir, path.basename(inFile, '.scss') + suffix);
  }

  // make sure it's an array.
  options.includePaths = argv['include-path'];
  if (!Array.isArray(options.includePaths)) {
    options.includePaths = [options.includePaths];
  }

  // include the image path.
  options.imagePath = argv['image-path'];

  // if it's an array, make it a string
  options.outputStyle = argv['output-style'];
  if (Array.isArray(options.outputStyle)) {
    options.outputStyle = options.outputStyle[0];
  }

  // if it's an array, make it a string
  options.sourceComments = argv['source-comments'];
  if (Array.isArray(options.sourceComments)) {
    options.sourceComments = options.sourceComments[0];
  }

  // Set the sourceMap path if the sourceComment was 'map', but set source-map was missing
  if (options.sourceComments === 'map' && !argv['source-map']) {
    argv['source-map'] = true;
  }

  // set source map file and set sourceComments to 'map'
  if (argv['source-map']) {
    options.sourceComments = 'map';
    if (argv['source-map'] === true) {
      options.sourceMap = outFile + '.map';
    } else {
      options.sourceMap = path.resolve(cwd, argv['source-map']);
    }
  }

  // watch directory for changes
  // compiled modified sass files and place in output directory
  // remove sass files from output directory
  if (argv.w) {
    watch(argv.w, (function(pattern, callback) {
        return function(filename) {
          if(pattern.test(filename)) {
            callback(filename);
          }
        };
      }(/\.(scss|sass)$/i, function(file){
        inFile = options.inFile = file;

        // src/path/to/file.scss => dest/path/to/
        var destDir = path.join(outDir, path.dirname(inFile.split('/').slice(1).join('/')));
        // file.scss => file.css
        var fileName = path.basename(inFile, path.extname(inFile)) + '.css';
        // dest/src/path/to/file.css
        outFile = options.outFile = path.join(destDir, fileName);

        if(fs.existsSync(inFile)) {
          emitter.emit('warn', '=> changed: ' + inFile);
          renderSync(options, emitter);
        } else {
          emitter.emit('warn', '=> removed: ' + inFile);
          try {
            fs.unlinkSync(outFile);
          } catch(e) {
            emitter.emit('warn', '=> not found: ' + inFile);
          }
        }
      })
    ));

    // Recursively scan a directory for files
    var walk = function(dir, done) {
      var results = [];
      fs.readdir(dir, function(err, list) {
        var pending = list.length;
        if (!pending) {
          return done(null, results);
        }
        list.forEach(function(file) {
          file = dir + '/' + file;
          fs.stat(file, function(err, stat) {
            if (stat && stat.isDirectory()) {
              walk(file, function(err, res) {
                results = results.concat(res);
                if (!--pending) {
                  done(null, results);
                }
              });
            } else {
              results.push(file);
              if (!--pending) {
                done(null, results);
              }
            }
          });
        });
      });
    };

    // Update utimes for each file
    walk(argv.w, function(err, files) {
      var now = Date.now();
      for(var i = 0; i < files.length; i++) {
        fs.utimes(files[i], now, now);
      }
    });
  } else {
    render(options, emitter);
  }

  return emitter;
};

exports.optimist = optimist;
