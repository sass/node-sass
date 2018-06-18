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
  // get global npm bin directory
  var globalBinPath = spawn.sync('npm', ['bin', '-g'], { stdout: 'inherit' }).stdout.toString().trim();
  var nodeGypExec = getNodeGyp();
  
  if (!nodeGypExec) {
    console.error('node-gyp not found!');
    process.exit(1);
  }
  
  var args = [nodeGypExec, 'rebuild', '--verbose'].concat(
    ['libsass_ext', 'libsass_cflags', 'libsass_ldflags', 'libsass_library'].map(function(subject) {
      return ['--', subject, '=', process.env[subject.toUpperCase()] || ''].join('');
    })).concat(options.args);

  console.log('Building:', [process.execPath].concat(args).join(' '));

  var proc = spawn(process.execPath, args, {
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
 * find node-gyp
 */

function getNodeGyp(){
  var globalBinPath = spawn.sync('npm', ['bin', '-g'], { stdout: 'inherit' }).stdout.toString().trim();
  var localBinPath = spawn.sync('npm', ['bin'], { stdout: 'inherit' }).stdout.toString().trim();
  var nodeGypExec = null;
  
  try{
    nodeGypExec = require.resolve(globalBinPath+'/node-gyp');
  }catch(errorGlobal){
    console.error('unable to resolve node-gyp globally!`');
  }
  if(!nodeGypExec){
    try{
      nodeGypExec = require.resolve(localBinPath+'/node-gyp');
    }catch(errorGlobal){
      console.error('unable to resolve node-gyp locally!`');
    }
  }

  if(!nodeGypExec){
    console.error('unable to resolve node-gyp. Try running `npm install node-gyp -g`.');
  }
  return nodeGypExec;
}

/**
 * Apply arguments and run
 */

testBinary(parseArgs(process.argv.slice(2)));
