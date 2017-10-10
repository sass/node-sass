/*!
 * node-sass: lib/binding.js
 */

var errors = require('./errors');
var path   = require('path');

/**
 * Require binding
 */
module.exports = function(ext) {
  var binaryPath = ext.getBinaryPath();
  if (!ext.hasBinary(binaryPath)) {
    if (!ext.isSupportedEnvironment()) {
      throw new Error(errors.unsupportedEnvironment());
    } else {
      throw new Error(errors.missingBinary());
    }
  }
  if (!path.isAbsolute(binaryPath)) {
    binaryPath = path.join('..', '..', '..', binaryPath);
  }
  return require(binaryPath);
};
