/*!
 * node-sass: scripts/build.js
 */

var fs = require('fs'),
  mkdir = require('mkdirp'),
  path = require('path'),
  spawn = require('cross-spawn'),
  sass = require('../lib/extensions');

/**
 * After build
 *
 * @param {Object} options
 * @api private
 */

function afterBuild(options) {
  var install = sass.getBinaryPath();
  var target = path.join(__dirname, '..', 'build',
    options.debug ? 'Debug' :
        process.config.target_defaults
            ?  process.config.target_defaults.default_configuration
            : 'Release',
    'binding.node');

  mkdir(path.dirname(install), function(err) {
    if (err && err.code !== 'EEXIST') {
      console.error(err.message);
      return;
    }

    fs.stat(target, function(err) {
      if (err) {
        console.error('Build succeeded but target not found');
        return;
      }

      fs.rename(target, install, function(err) {
        if (err) {
          console.error(err.message);
          return;
        }

        console.log('Installed to', install);
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
  var nodeGyp = resolveNodeGyp();
  var nodeGypArgs = nodeGyp.args.concat([
    'rebuild',
    '--verbose',
    '--libsass_ext=' + (process.env['LIBSASS_EXT'] || ''),
    '--libsass_cflags=' + (process.env['LIBSASS_CFLAGS'] || ''),
    '--libsass_ldflags=' + (process.env['LIBSASS_LDFLAGS'] || ''),
    '--libsass_library=' + (process.env['LIBSASS_LIBRARY'] || ''),
  ])
  .concat(options.args);

  console.log(['Building:', nodeGyp.exeName].concat(nodeGypArgs).join(' '));

  var proc = spawn(nodeGyp.exeName, nodeGypArgs, {
    stdio: [0, 1, 2]
  });

  proc.on('exit', function(errorCode) {
    if (!errorCode) {
      afterBuild(options);
      return;
    }

    if (errorCode === 127 ) {
      console.error('node-gyp not found!');
    } else {
      console.error('Build failed with error code:', errorCode);
    }

    process.exit(1);
  });
}

/**
 * Resolve node-gyp command to invoke
 *
 * @api private
 */
function resolveNodeGyp() {
  if (process.jsEngine === 'chakracore') {
    // For node-chakracore, check if node-gyp is in the path.
    // If yes, use it instead of using node-gyp directly from
    // node_modules because the one in node_modules is not
    // compatible with node-chakracore.
    var nodePath = path.dirname(process.execPath);
    var nodeGypName = process.platform === 'win32' ? 'node-gyp.cmd' : 'node-gyp';
    var globalNodeGypBin = process.env.Path.split(';').filter(function(envPath) {
      return envPath.startsWith(nodePath) &&
               envPath.endsWith('node-gyp-bin') &&
               fs.existsSync(path.resolve(envPath, nodeGypName));
    });

    if (globalNodeGypBin.length !== 0) {
      return { exeName: 'node-gyp', args: [] };
    }

    console.error('node-gyp incompatible with node-chakracore! Please use node-gyp installed with node-chakracore.');
    process.exit(1);
  }

  var localNodeGypBin = require.resolve(path.join('node-gyp', 'bin', 'node-gyp.js'));

  return {
    exeName: process.execPath,
    args: [localNodeGypBin],
  };
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
    platform: process.platform,
    force: process.env.npm_config_force === 'true',
  };

  options.args = args.filter(function(arg) {
    if (arg === '-f' || arg === '--force') {
      options.force = true;
      return false;
    } else if (arg.substring(0, 13) === '--target_arch') {
      options.arch = arg.substring(14);
    } else if (arg === '-d' || arg === '--debug') {
      options.debug = true;
    } else if (arg.substring(0, 13) === '--libsass_ext' && arg.substring(14) !== 'no') {
      options.libsassExt = true;
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

  if (!sass.hasBinary(sass.getBinaryPath())) {
    return build(options);
  }

  console.log('Binary found at', sass.getBinaryPath());
  console.log('Testing binary');

  try {
    require('../').renderSync({
      data: 's { a: ss }'
    });

    console.log('Binary is fine');
  } catch (e) {
    console.log('Binary has a problem:', e);
    console.log('Building the binary locally');

    return build(options);
  }
}

/**
 * Apply arguments and run
 */

testBinary(parseArgs(process.argv.slice(2)));
