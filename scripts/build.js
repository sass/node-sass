/*!
 * node-sass: scripts/build.js
 */

var fs = require('fs'),
  mkdir = require('mkdirp'),
  path = require('path'),
  spawn = require('cross-spawn'),
  sass = require('../lib/extensions'),
  Constants = require('../lib/constants'),
  yargs = require('yargs');

/**
 * Build
 *
 * @param {Object} gypOptions
 * @api private
 */

function build(gypOptions, callback) {
  [
    'libsass_ext',
    'libsass_cflags',
    'libsass_ldflags',
    'libsass_library'
  ].forEach(function (sassflag) {
    gypOptions[sassflag] = process.env[sassflag.toUpperCase()];
  });

  var args = [
    require.resolve(path.join('node-gyp', 'bin', 'node-gyp.js')),
    'rebuild',
    '--verbose'
  ].concat(
      Object.entries(gypOptions).filter(function(prop) { return prop[1]; }).map(function (prop) {
        return '--' + prop[0] + (prop[1] === true ? '' : '=' + prop[1]);
      })
    );

  console.log('Building:', [process.execPath].concat(args).join(' '));

  var proc = spawn(process.execPath, args, {
    stdio: [0, 1, 2]
  });

  proc.on('exit', callback);

}

/**
 * Parse arguments
 *
 * @param {Array} args
 * @api private
 */

/**
 * Test for pre-built library
 *
 * @param {Object} options
 * @api private
 */

/**
 * Apply arguments and run
 */

var argv = Object.assign(Constants.DefaultOptions, yargs
  .option('jobs', {
    alias: 'j',
    describe: 'Run make in parallel'
  })
  .option('target', {
    describe: 'Node.js version to build for (default is process.version)'
  })
  .option('force', {
    alias: 'f',
    describe: 'Rebuild even if file already exists'
  })
  .option('arch', {
    alias: 'a'
  })
  .option('modules-version', {
    alias: 'm',
    describe: 'Node module version to build for (default is process.versions.modules)'
  })
  .option('debug', {
    alias: 'd'
  }).argv);

var BinaryPath = sass.getBinaryPath(argv);
if (!argv.force && fs.existsSync(BinaryPath)) {
  console.log('Binary found at', BinaryPath);
  process.exit(0);
}

var gypOptions = {
  arch: argv.arch,
  jobs: argv.jobs,
  target: argv.target,
  debug: argv.debug
};
var ModuleDetails = Constants.ModuleVersions[argv.modulesVersion];
if (!ModuleDetails) {
  console.error('Unknown Node Modules Version: ' + argv.modulesVersion);
  process.exit(1);
}
if (ModuleDetails[0] === Constants.Runtimes.ELECTRON) {
  gypOptions['dist-url'] = 'https://atom.io/download/electron';
  argv.arch = gypOptions.arch = process.platform === 'win32' ? 'ia32' : process.arch;
}

build(gypOptions, function (errorCode) {
  if (errorCode) {
    if (errorCode === 127) {
      console.error('node-gyp not found!');
    } else {
      console.error('Build failed with error code:', errorCode);
    }
  
    process.exit(1);
  }
  var install = BinaryPath;
  var target = path.join(
      __dirname,
      '..',
      'build',
      gypOptions.debug ? 'Debug' : 'Release',
      'binding.node'
    );
  
  mkdir(path.dirname(install), function (err) {
    if (err && err.code !== 'EEXIST') {
      return console.error(err.message);
    }
  
    fs.stat(target, function (err) {
      if (err) {
        return console.error('Build succeeded but target not found');
      }
  
      fs.rename(target, install, function (err) {
        if (err) {
          return console.error(err.message);
        }
  
        console.log('Installed to', install);
      });
    });
  });
});