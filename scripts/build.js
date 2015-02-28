var eol = require('os').EOL,
    fs = require('fs'),
    mkdir = require('mkdirp'),
    path = require('path'),
    spawn = require('child_process').spawn;

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
  var arguments = [path.join('node_modules', 'pangyp', 'bin', 'node-gyp'), 'rebuild'].concat(options.args);

  console.log(['Building:', process.runtime.execPath].concat(arguments).join(' '));

  var proc = spawn(process.runtime.execPath, arguments, {
    stdio: [0, 1, 2]
  });

  proc.on('exit', function(errorCode) {
    if (!errorCode) {
      afterBuild(options);

      return;
    }

    console.error(errorCode === 127 ? 'node-gyp not found!' : 'Build failed');
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

    console.log('`' + process.sassBinaryName + '` exists; testing.');

    try {
      require('../').renderSync({
        data: 's: { a: ss }'
      });

      console.log('Binary is fine; exiting.');
    } catch (e) {
      console.log(['Problem with the binary.', 'Manual build incoming.'].join(eol));

      return build(options);
    }
  });
}

/**
 * Apply arguments and run
 */

testBinary(parseArgs(process.argv.slice(2)));
