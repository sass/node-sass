var fs = require('fs');

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
  return [process.platform, '-',
          process.arch, '-',
          process.versions.v8].join('');
}

process.runtime = getRuntimeInfo();
process.sassBinaryName = getBinaryIdentifiableName();
