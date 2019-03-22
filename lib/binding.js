/*!
 * node-sass: lib/binding.js
 */

var errors = require('./errors'),
  Constants = require('./constants'),
  DefaultOptions = Constants.DefaultOptions;
/**
 * Require binding
 */
module.exports = function(ext) {
  if (!ext.hasBinary(ext.getBinaryPath(DefaultOptions))) {
    if (!ext.isSupportedEnvironment(process.platform, DefaultOptions.arch, DefaultOptions.modulesVersion)) {
      throw new Error(errors.unsupportedEnvironment(DefaultOptions));
    } else {
      throw new Error(errors.missingBinary(DefaultOptions));
    }
  }

  return require(ext.getBinaryPath(DefaultOptions));
};
