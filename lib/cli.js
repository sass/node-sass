var watch = require('node-watch'),
    render = require('./render'),
    path = require('path'),
    Emitter = require('events').EventEmitter,
    stdin = require('get-stdin'),
    cwd = process.cwd();

var yargs = require('yargs')
  .usage('Compile .scss files with node-sass.\nUsage: $0 [options] <input.scss> [<output.css>]')
  .version(require('../package.json').version, 'version').alias('version', 'V')
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
  .options('precision', {
    describe: 'The amount of precision allowed in decimal numbers',
    'default': 5
  })
  .options('watch', {
    describe: 'Watch a directory or file',
    alias: 'w',
    type: 'boolean'
  })
  .options('output', {
    describe: 'Output css file',
    alias: 'o'
  })
  .options('stdout', {
    describe: 'Print the resulting CSS to stdout',
    type: 'boolean'
  })
  .options('omit-source-map-url', {
      describe: 'Omit source map URL comment from output',
      type: 'boolean',
      alias: 'x'
  })
  .options('indented-syntax', {
      describe: 'Treat data from stdin as sass code (versus scss)',
      type: 'boolean',
      alias: 'i'
  })
  .options('help', {
    describe: 'Print usage info',
    type: 'string',
    alias: 'help'
  })
  .check(function(argv) {
    if (argv.help) { return true; }
    if (!argv._.length && (process.stdin.isTTY || process.env.isTTY)) {
      return false;
    }
  });

// throttle function, used so when multiple files change at the same time
// (e.g. git pull) the files are only compiled once.
function throttle(fn) {
  var timer;
  var args = Array.prototype.slice.call(arguments, 1);

  return function() {
    var self = this;
    clearTimeout(timer);
    timer = setTimeout(function() {
      fn.apply(self, args);
    }, 20);
  };
}

function isSassFile(file) {
  return file.match(/\.(sass|scss)/);
}

function run(options, emitter) {
  if (!Array.isArray(options.includePaths)) {
    options.includePaths = [options.includePaths];
  }

  if (Array.isArray(options.outputStyle)) {
    options.outputStyle = options.outputStyle[0];
  }

  if (Array.isArray(options.sourceComments)) {
    options.sourceComments = options.sourceComments[0];
  }

  if (options.sourceMap) {
    if (options.sourceMap === true) {
      options.sourceMap = options.dest + '.map';
    } else {
      options.sourceMap = path.resolve(cwd, options.sourceMap);
    }
  }

  if (options.watch) {
    var throttledRender = throttle(render, options, emitter);
    var watchDir = options.watch;

    if (watchDir === true) {
      watchDir = [];
    } else if (!Array.isArray(watchDir)) {
      watchDir = [watchDir];
    }

    watchDir.push(options.src);

    watch(watchDir, function(file) {
      emitter.emit('warn', '=> changed: '.grey + file.blue);

      if (isSassFile(file)) {
        throttledRender();
      }
    });

    throttledRender();
  } else {
    render(options, emitter);
  }
}

module.exports = function(args) {
  var argv = yargs.parse(args);
  var emitter = new Emitter();
  var options = {
    imagePath: argv['image-path'],
    includePaths: argv['include-path'],
    omitSourceMapUrl: argv['omit-source-map-url'],
    indentedSyntax: argv['indented-syntax'],
    outputStyle: argv['output-style'],
    precision: argv.precision,
    sourceComments: argv['source-comments'],
    sourceMap: argv['source-map'],
    stdout: argv.stdout,
    watch: argv.w
  };

  if (argv.help) {
    yargs.showHelp();
    process.exit();
    return;
  }

  emitter.on('error', function(err) {
    console.error(err);
    process.exit(1);
  });

  options.src = argv._[0];
  options.dest = argv.o || argv._[1];

  if (!options.dest && (process.stdout.isTTY || process.env.isTTY)) {
    var suffix = '.css';
    if (/\.css$/.test(options.src)) {
      suffix = '';
    }
    options.dest = path.join(cwd, path.basename(options.src, '.scss') + suffix);
  }

  if (process.stdin.isTTY || process.env.isTTY) {
    run(options, emitter);
  } else {
    stdin(function(data) {
      options.data = data;
      run(options, emitter);
    });
  }

  return emitter;
};

module.exports.yargs = yargs;
