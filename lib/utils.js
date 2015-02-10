var semver = require('semver'),
    runtimeVersion = semver.parse(process.version);

/**
 * Get Runtime Name
 *
 * @api private
 */

function getRuntimeName() {
  return process.execPath
        .split(/[\\/]+/).pop()
        .split('.')[0];
}

/**
 * Get unique name of binary for current platform
 *
 * @api public
 */

module.exports.getBinaryIdentifiableName = function() {
  return process.platform + '-' +
         process.arch + '-' +
         getRuntimeName() + '-' +
         runtimeVersion.major + '.' +
         runtimeVersion.minor;
};
