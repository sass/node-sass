var eol = require('os').EOL,
    fs = require('fs'),
    package = require('../package.json');

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
 * Get unique name of binary for current platform
 *
 * @api private
 */

function getBinaryIdentifiableName() {
  var v8SemVersion = process.versions.v8.split('.').slice(0, 3).join('.');

  return [process.platform, '-',
          process.arch, '-',
          v8SemVersion].join('');
}

function getSassInfo() {
  return [
           ['node-sass', package.version, '(Wrapper)', '[JavaScript]'].join('\t'),
           ['libsass  ', package.libsass, '(Sass Compiler)', '[C/C++]'].join('\t'),
         ].join(eol);
}

process.runtime = getRuntimeInfo();
process.sassInfo = getSassInfo();
process.sassBinaryName = getBinaryIdentifiableName();
