/*!
 * node-sass: lib/binding.js
 */

var errors = require('./errors'),
  Constants = require('./constants');
/**
 * Require binding
 */
module.exports = function(ext) {
  if (!ext.hasBinary(ext.getBinaryPath(Constants.DefaultOptions))) {
    if (!ext.isSupportedEnvironment()) {
      throw new Error(errors.unsupportedEnvironment());
    } else {
      throw new Error(errors.missingBinary());
    }
  }

  return require(ext.getBinaryPath(Constants.DefaultOptions));
};
