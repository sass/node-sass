/*!
 * node-sass: lib/binding.js
 */

var errors = require('./errors');

/**
 * Require binding
 */
module.exports = function(ext) {
  var binaryPath = ext.getBinaryPath()

  if (!ext.hasBinary(binaryPath)) {
    if (!ext.isSupportedEnvironment()) {
      throw new Error(errors.unsupportedEnvironment());
    } else {
      throw new Error(errors.missingBinary());
    }
  }

  return require(binaryPath);
};
