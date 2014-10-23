var watch = require('node-watch'),
    render = require('./render'),
    path = require('path'),
    Emitter = require('events').EventEmitter,
    stdin = require('get-stdin'),
    meow = require('meow'),
    cwd = process.cwd();

function init(args) {
  return meow({
    argv: args,
    pkg: '../package.json',
    help: [
      '  Usage',
      '    node-sass [options] <input.scss> [output.css]',
      '    cat <input.scss> | node-sass > output.css',
      '',
      '  Example',
      '    node-sass --output-style compressed foobar.scss foobar.css',
      '    cat foobar.scss | node-sass --output-style compressed > foobar.css',
      '',
      '  Options',
      '    -w, --watch                Watch a directory or file',
      '    -o, --output               Output CSS file',
      '    -x, --omit-source-map-url  Omit source map URL comment from output',
      '    -i, --indented-syntax      Treat data from stdin as sass code (versus scss)',
      '    --output-style             CSS output style (nested|expanded|compact|compressed)',
      '    --source-comments          Include debug info in output (none|normal|map)',
      '    --source-map               Emit source map',
      '    --include-path             Path to look for imported files',
      '    --image-path               Path to prepend when using the `image-url()` helper',
      '    --precision                The amount of precision allowed in decimal numbers',
      '    --stdout                   Print the resulting CSS to stdout',
      '    --help                     Print usage info'
    ].join('\n')
  }, {
    boolean: [
      'indented-syntax',
      'omit-source-map-url',
      'stdout',
      'watch'
    ],
    string: [
      'image-path',
      'include-path',
      'output',
      'output-style',
      'precision',
      'source-comments'
    ],
    alias: {
      i: 'indented-syntax',
      o: 'output',
      w: 'watch',
      x: 'omit-source-map-url'
    },
    default: {
      'image-path': '',
      'include-path': cwd,
      'output-style': 'nested',
      precision: 5,
      'source-comments': 'none'
    }
  });
}

function throttle(fn) {
  var timer;
  var args = [].slice.call(arguments, 1);

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
  if (!Array.isArray(options.includePath)) {
    options.includePath = [options.includePath];
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
  var cli = init(args);
  var emitter = new Emitter();
  var input = cli.input;
  var options = cli.flags;

  emitter.on('error', function(err) {
    console.error(err);
    process.exit(1);
  });

  options.src = input[0] || null;
  options.dest = options.output || input[1] || null;

  if (!options.dest && (process.stdout.isTTY || process.env.isTTY)) {
    var suffix = '.css';
    if (/\.css$/.test(options.src)) {
      suffix = '';
    }
    options.dest = path.join(cwd, path.basename(options.src, '.scss') + suffix);
  }

  if (process.stdin.isTTY || process.env.isTTY) {
    if (!input.length) {
      console.error([
        'Provide a sass file to render',
        '',
        '  Example',
        '    node-sass --output-style compressed foobar.scss foobar.css',
        '    cat foobar.scss | node-sass --output-style compressed > foobar.css'
      ].join('\n'));
      process.exit(1);
    }
    run(options, emitter);
  } else {
    stdin(function(data) {
      options.data = data;
      run(options, emitter);
    });
  }

  return emitter;
};
