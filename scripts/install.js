/*!
 * node-sass: scripts/install.js
 */

var fs = require('fs'),
    eol = require('os').EOL,
    mkdir = require('mkdirp'),
    path = require('path'),
    got = require('got'),
    pkg = require('../package.json'),
    sass = require('../lib/extensions');

/**
 * Download file, if succeeds save, if not delete
 *
 * @param {String} url
 * @param {String} dest
 * @param {Function} cb
 * @api private
 */

function download(url, dest, cb) {
  var reportError = function(err) {
    cb(['Cannot download "', url, '": ', eol, eol,
      typeof err.message === 'string' ? err.message : err, eol, eol,
      'Hint: If github.com is not accessible in your location', eol,
      '      try setting a proxy via HTTP_PROXY, e.g. ', eol, eol,
      '      export HTTP_PROXY=http://example.com:1234',eol, eol,
      'or configure npm proxy via', eol, eol,
      '      npm config set proxy http://example.com:8080'].join(''));
  };

  applyProxy({ rejectUnauthorized: false }, function(options) {
    options.headers = {
      'User-Agent': [
        'node/', process.version, ' ',
        'node-sass-installer/', pkg.version
      ].join('')
    };
    try {
      got.stream(url, options)
        .on('error', function(error) {
          reportError(error);
        })
        .on('end', function() {
          cb();
        })
        .pipe(fs.createWriteStream(dest));
    } catch (err) {
      cb(err);
    }
  });
}

/**
 * Get applyProxy settings
 *
 * @param {Object} options
 * @param {Function} cb
 * @api private
 */

function applyProxy(options, cb) {
  var env = process.env;

  options.proxy = env.npm_config_https_proxy ||
                  env.npm_config_proxy ||
                  env.npm_config_http_proxy ||
                  env.HTTPS_PROXY ||
                  env.https_proxy ||
                  env.HTTP_PROXY ||
                  env.http_proxy;

  cb(options);
}

/**
 * Check and download binary
 *
 * @api private
 */

function checkAndDownloadBinary() {
  if (sass.hasBinary(sass.getBinaryPath())) {
    return;
  }

  mkdir(path.dirname(sass.getBinaryPath()), function(err) {
    if (err) {
      console.error(err);
      return;
    }

    download(sass.getBinaryUrl(), sass.getBinaryPath(), function(err) {
      if (err) {
        console.error(err);
        return;
      }

      console.log('Binary downloaded and installed at', sass.getBinaryPath());
    });
  });
}

/**
 * Skip if CI
 */

if (process.env.SKIP_SASS_BINARY_DOWNLOAD_FOR_CI) {
  console.log('Skipping downloading binaries on CI builds');
  return;
}

/**
 * If binary does not exist, download it
 */

checkAndDownloadBinary();
