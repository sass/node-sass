/*!
 * node-sass: lib/extensions.js
 */

var eol = require('os').EOL,
    flags = {},
    fs = require('fs'),
    package = require('../package.json'),
    path = require('path');

/**
 * Collect Arguments
 *
 * @param {Array} args
 * @api private
 */

function collectArguments(args) {
  for (var i = 0; i < args.length; i += 2) {
    if (args[i].lastIndexOf('--', 0) !== 0) {
      --i;
      continue;
    }

    flags[args[i]] = args[i + 1];
  }
}

/**
 * Get Runtime Info
 *
 * @api private
 */

function getRuntimeInfo() {
  var execPath = fs.realpathSync(process.execPath); // resolve symbolic link

  var runtime = execPath
               .split(/[\\/]+/).pop()
               .split('.').shift();

  runtime = runtime === 'nodejs' ? 'node' : runtime;

  return {
    name: runtime,
    execPath: execPath
  };
}

/**
 * Get binary name.
 * If environment variable SASS_BINARY_NAME or
 * process aurgument --binary-name is provide,
 * return it as is, otherwise make default binary
 * name: {platform}-{arch}-{v8 version}.node
 *
 * @api private
 */

function getBinaryName() {
  var binaryName;

  if (flags['--sass-binary-name']) {
    binaryName = flags['--sass-binary-name'];
  } else if (package.nodeSassConfig && package.nodeSassConfig.binaryName) {
    binaryName = package.nodeSassConfig.binaryName;
  } else if (process.env.SASS_BINARY_NAME) {
    binaryName = process.env.SASS_BINARY_NAME;
  } else {
    binaryName = [process.platform, '-',
                  process.arch, '-',
                  process.versions.modules].join('');
  }

  return [binaryName, 'binding.node'].join('_');
}

/**
 * Retrieve the URL to fetch binary.
 * If environment variable SASS_BINARY_URL
 * is set, return that path. Otherwise make
 * path using current release version and
 * binary name.
 *
 * @api private
 */

function getBinaryUrl() {
  return flags['--sass-binary-url'] ||
         package.nodeSassConfig ? package.nodeSassConfig.binaryUrl : null ||
         process.env.SASS_BINARY_URL ||
         ['https://github.com/xzyfer/node-sass/releases/download/v',
          package.version, '/', sass.binaryName].join('');
}

/**
 * Get Sass version information
 *
 * @api private
 */

function getVersionInfo() {
  return [
           ['node-sass', package.version, '(Wrapper)', '[JavaScript]'].join('\t'),
           ['libsass  ', package.libsass, '(Sass Compiler)', '[C/C++]'].join('\t'),
  ].join(eol);
}

collectArguments(process.argv.slice(2));

var sass = process.sass = {};

sass.binaryName = getBinaryName();
sass.binaryUrl = getBinaryUrl();
sass.runtime = getRuntimeInfo();
sass.versionInfo = getVersionInfo();

/**
 * Get binary path.
 * If environment variable SASS_BINARY_PATH or
 * process aurgument --binary-path is provide,
 * select it by appending binary name, otherwise
 * make default binary path using binary name.
 * Once the primary selection is made, check if
 * callers wants to throw if file not exists before
 * returning.
 *
 * @param {Boolean} throwIfNotExists
 * @api private
 */

sass.getBinaryPath = function(throwIfNotExists) {
  var binaryPath;

  if (flags['--sass-binary-path']) {
    binaryPath = flags['--sass-binary-path'];
  } else if (package.nodeSassConfig && package.nodeSassConfig.binaryPath) {
    binaryPath = package.nodeSassConfig.binaryPath;
  } else if (process.env.SASS_BINARY_PATH) {
    binaryPath = process.env.SASS_BINARY_PATH;
  } else {
    binaryPath = path.join(__dirname, '..', 'vendor', sass.binaryName.replace(/_/, '/'));
  }

  if (!fs.existsSync(binaryPath) && throwIfNotExists) {
    throw new Error('`libsass` bindings not found. Try reinstalling `node-sass`?');
  }

  return binaryPath;
};

sass.binaryPath = sass.getBinaryPath();
