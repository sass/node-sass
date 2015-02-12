var semver = require('semver'),
    runtimeVersion = semver.parse(process.version);

/**
 * Get Runtime Name
 *
 * @api private
 */

function getRuntimeName() {
  var runtime = process.execPath
        .split(/[\\/]+/).pop()
        .split('.').shift();

  return runtime === 'node' || runtime === 'nodejs' ? 'node' : runtime;
}

/**
 * Get unique name of binary for current platform
 *
 * @api private
 */

function getBinaryIdentifiableName() {
  return [process.platform, '-',
          process.arch, '-',
          getRuntimeName(), '-',
          runtimeVersion.major, '.',
          runtimeVersion.minor].join('');
}

process.runtime = getRuntimeName();
process.sassBinaryName = getBinaryIdentifiableName();
