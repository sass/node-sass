/*!
 * node-sass: scripts/install.js
 */

var fs = require('fs'),
  eol = require('os').EOL,
  mkdir = require('mkdirp'),
  path = require('path'),
  sass = require('../lib/extensions'),
  request = require('request'),
  log = require('npmlog'),
  downloadOptions = require('./util/downloadoptions');

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
    var timeoutMessge;

    if (err.code === 'ETIMEDOUT') {
      if (err.connect === true) {
        // timeout is hit while your client is attempting to establish a connection to a remote machine
        timeoutMessge = 'Timed out attemping to establish a remote connection';
      } else {
        timeoutMessge = 'Timed out whilst downloading the prebuilt binary';
        // occurs any time the server is too slow to send back a part of the response
      }

    }
    cb(['Cannot download "', url, '": ', eol, eol,
      typeof err.message === 'string' ? err.message : err, eol, eol,
      timeoutMessge ? timeoutMessge + eol + eol : timeoutMessge,
      'Hint: If github.com is not accessible in your location', eol,
      '      try setting a proxy via HTTP_PROXY, e.g. ', eol, eol,
      '      export HTTP_PROXY=http://example.com:1234',eol, eol,
      'or configure npm proxy via', eol, eol,
      '      npm config set proxy http://example.com:8080'].join(''));
  };

  var successful = function(response) {
    return response.statusCode >= 200 && response.statusCode < 300;
  };

  log.http('node-sass install', 'Start downloading binary at' + url);

  try {
    request(url, downloadOptions(), function(err, response) {
      if (err) {
        reportError(err);
      } else if (!successful(response)) {
        reportError(['HTTP error', response.statusCode, response.statusMessage].join(' '));
      } else {
        cb();
      }
    })
    .on('response', function(response) {
      var length = parseInt(response.headers['content-length'], 10);
      var progress = log.newItem(url, length);

      if (successful(response)) {
        response.pipe(fs.createWriteStream(dest));
      }

      // The `progress` is true by default. However if it has not
      // been explicitly set it's `undefined` which is considered
      // as far as npm is concerned.
      if (process.env.npm_config_progress !== false) {
        log.enableProgress();

        response.on('data', function(chunk) {
          progress.completeWork(chunk.length);
        })
        .on('end', progress.finish);
      }
    });
  } catch (err) {
    cb(err);
  }
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
      log.error('node-sass install', err);
      return;
    }

    download(sass.getBinaryUrl(), sass.getBinaryPath(), function(err) {
      if (err) {
        log.error('node-sass install', err);
        return;
      }

      log.info('node-sass install', 'Binary downloaded and installed at' + sass.getBinaryPath());
    });
  });
}

/**
 * Skip if CI
 */

if (process.env.SKIP_SASS_BINARY_DOWNLOAD_FOR_CI) {
  log.info('node-sass install', 'Skipping downloading binaries on CI builds');
  return;
}

/**
 * If binary does not exist, download it
 */

checkAndDownloadBinary();
