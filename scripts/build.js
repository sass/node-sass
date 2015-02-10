var fs = require('fs'),
    path = require('path'),
    spawn = require('child_process').spawn,
    mkdir = require('mkdirp'),
    Mocha = require('mocha');
    
require('../lib/extensions');

/**
 * After build
 *
 * @param {Object} options
 * @api private
 */

function afterBuild(options) {
  var folder = options.debug ? 'Debug' : 'Release';
  var target = path.join(__dirname, '..', 'build', folder, 'binding.node');
  var install = path.join(__dirname, '..', 'vendor', process.sassBinaryName, 'binding.node');

  mkdir(path.join(__dirname, '..', 'vendor', process.sassBinaryName), function (err) {
    if (err && err.code !== 'EEXIST') {
      console.error(err.message);
      return;
    }

    fs.stat(target, function (err) {
      if (err) {
        console.error('Build succeeded but target not found');
        return;
      }

      fs.rename(target, install, function (err) {
        if (err) {
          console.error(err.message);
          return;
        }

        console.log('Installed in `' + install + '`');
      });
    });
  });
}

/**
 * Build
 *
 * @param {Object} options
 * @api private
 */

function build(options) {
  var proc = spawn(process.execPath, ['node_modules/pangyp/bin/node-gyp', 'rebuild'].concat(options.args), {
    stdio: [0, 1, 2]
  });

  proc.on('exit', function(code) {
    if (code) {
      if (code === 127) {
        console.error([
          'node-gyp not found! Please upgrade your install of npm!',
          'You need at least 1.1.5 (I think) and preferably 1.1.30.'
        ].join(' '));

        return;
      }

      console.error('Build failed');
      return;
    }

    afterBuild(options);
  });
}

/**
 * Parse arguments
 *
 * @param {Array} args
 * @api private
 */

function parseArgs(args) {
  var options = {
    arch: process.arch,
    platform: process.platform
  };

  options.args = args.filter(function(arg) {
    if (arg === '-f' || arg === '--force') {
      options.force = true;
      return false;
    } else if (arg.substring(0, 13) === '--target_arch') {
      options.arch = arg.substring(14);
    } else if (arg === '-d' || arg === '--debug') {
      options.debug = true;
    }

    return true;
  });

  return options;
}

/**
 * Test for pre-built library
 *
 * @param {Object} options
 * @api private
 */

function testBinary(options) {
  if (options.force || process.env.SASS_FORCE_BUILD) {
    return build(options);
  }

  fs.stat(path.join(__dirname, '..', 'vendor', process.sassBinaryName, 'binding.node'), function (err) {
    if (err) {
      return build(options);
    }

    console.log('`' + process.sassBinaryName + '` exists; testing');

    var mocha = new Mocha({
      ui: 'bdd',
      timeout: 999999,
      reporter: function() {}
    });

    mocha.addFile(path.resolve(__dirname, '..', 'test', 'api.js'));
    mocha.grep(/should compile sass to css with file/).run(function (done) {
      if (done !== 0) {
        console.log([
          'Problem with the binary.',
          'Manual build incoming.',
          'Please consider contributing the release binary to https://github.com/sass/node-sass-binaries for npm distribution.'
        ].join('\n'));

        return build(options);
      }

      console.log('Binary is fine; exiting');
    });
  });
}

/**
 * Apply arguments and run
 */

testBinary(parseArgs(process.argv.slice(2)));
