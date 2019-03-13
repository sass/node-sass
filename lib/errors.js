/*!
 * node-sass: lib/errors.js
 */

var sass = require('./extensions'),
  pkg = require('../package.json');

function humanEnvironment(options) {
  return sass.getHumanEnvironment(sass.getBinaryName(options));
}

function foundBinaries() {
  return [
    'Found bindings for the following environments:',
    foundBinariesList(),
  ].join('\n');
}

function foundBinariesList() {
  return sass.getInstalledBinaries().map(function(env) {
    return '  - ' + sass.getHumanEnvironment(env);
  }).join('\n');
}

function missingBinaryFooter() {
  return [
    'This usually happens because your environment has changed since running `npm install`.',
    'Run `npm rebuild node-sass` to download the binding for your current environment.',
  ].join('\n');
}

module.exports.unsupportedEnvironment = function(options) {
  return [
    'Node Sass does not yet support your current environment: ' + humanEnvironment(options),
    'For more information on which environments are supported please see:',
    'https://github.com/sass/node-sass/releases/tag/v' + pkg.version
  ].join('\n');
};

module.exports.missingBinary = function(options) {
  return [
    'Missing binding ' + sass.getBinaryPath(options),
    'Node Sass could not find a binding for your current environment: ' + humanEnvironment(options),
    '',
    foundBinaries(),
    '',
    missingBinaryFooter(),
  ].join('\n');
};
