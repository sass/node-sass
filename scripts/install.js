/*!
 * node-sass: scripts/install.js
 */

var fs = require('fs'),
    eol = require('os').EOL,
    mkdir = require('mkdirp'),
    path = require('path'),
    sass = require('../lib/extensions'),
    request = require('request'),
    pkg = require('../package.json');

/**
 * Download file, if succeeds save, if not delete
 *
 * @param {String} src
 * @param {String} dest
 * @param {Function} cb
 * @api private
 */

function copy(src, dest, cb) {
  var reportError = function(err) {
    cb(['Cannot copy "', src, '": ', eol, eol,
      typeof err.message === 'string' ? err.message : err, eol, eol,
      'Hint: If github.com is not accessible in your location', eol,
      '      try setting a proxy via HTTP_PROXY, e.g. ', eol, eol,
      '      export HTTP_PROXY=http://example.com:1234',eol, eol,
      'or configure npm proxy via', eol, eol,
      '      npm config set proxy http://example.com:8080'].join(''));
  };

  try {
    fs.createReadStream(src)
      .pipe(fs.createWriteStream(dest));
    cb();
  } catch (err) {
    cb(err);
  }
}

/**
 * Determine local proxy settings
 *
 * @param {Object} options
 * @param {Function} cb
 * @api private
 */

function getProxy() {
  return process.env.npm_config_https_proxy ||
         process.env.npm_config_proxy ||
         process.env.npm_config_http_proxy ||
         process.env.HTTPS_PROXY ||
         process.env.https_proxy ||
         process.env.HTTP_PROXY ||
         process.env.http_proxy;
}

/**
 * Check and copy binary
 *
 * @api private
 */

function checkAndCopyBinary() {
  if (sass.hasBinary(sass.getBinaryPath())) {
    return;
  }

  mkdir(path.dirname(sass.getBinaryPath()), function(err) {
    if (err) {
      console.error(err);
      return;
    }

    copy(sass.getBindingPath(), sass.getBinaryPath(), function(err) {
      if (err) {
        console.error(err);
        return;
      }

      console.log('Binary copied and installed at', sass.getBinaryPath());
    });
  });
}

/**
 * Skip if CI
 */

if (process.env.SKIP_SASS_BINARY_DOWNLOAD_FOR_CI) {
  console.log('Skipping copying binaries on CI builds');
  return;
}

/**
 * If binary does not exist, copy it
 */

checkAndCopyBinary();
