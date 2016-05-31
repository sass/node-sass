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

  var successful = function(response) {
    return response.statusCode >= 200 && response.statusCode < 300;
  };

  var options = {
    rejectUnauthorized: false,
    proxy: getProxy(),
    headers: {
      'User-Agent': getUserAgent(),
    }
  };

  try {
    request(url, options, function(err, response) {
      if (err) {
        reportError(err);
      } else if (!successful(response)) {
          reportError(['HTTP error', response.statusCode, response.statusMessage].join(' '));
      } else {
          cb();
      }
    })
    .on('response', function(response) {
        if (successful(response)) {
          response.pipe(fs.createWriteStream(dest));
        }
    });
  } catch (err) {
    cb(err);
  }
}

/**
 * A custom user agent use for binary downloads.
 *
 * @api private
 */
function getUserAgent() {
  return [
    'node/', process.version, ' ',
    'node-sass-installer/', pkg.version
  ].join('');
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

    var tmpPath = getTempPath(sass.getBinaryName());
    if (!(sass.hasBinary(tmpPath))) { // download and install
      download(sass.getBinaryUrl(), tmpPath, function(err) {
        if (err) {
          console.error(err);
          return;
        }
        console.log('Binary downloaded at', tmpPath);
        copyBinary(tmpPath, sass.getBinaryPath());
      });

    } else { // install only
      copyBinary(tmpPath, sass.getBinaryPath());
    }
  });
}

/**
 * Find a temp folder for file
 *
 * @param {String} binaryName
 * @returns {string}
 * @api private
 */

function getTempPath(binaryName) {
  var candidateTmpDirs = [
    process.env.TMPDIR || process.env.TEMP || process.env.npm_config_tmp,
    '/tmp',
    path.join(process.cwd(), 'tmp')
  ];

  for (var i = 0; i < candidateTmpDirs.length; i++) {
    var candidatePath = path.join(candidateTmpDirs[i], 'node-sass', 'releases', 'download', 'v' + pkg.version);

    try {
      mkdir.sync(candidatePath);
      return path.join(candidatePath, binaryName);
    } catch (err) {
      console.error(candidatePath, 'is not writable:', err.message);
    }
  }
}

/**
 * Copy file
 *
 * @param tmp
 * @param dest
 * @api private
 */
function copyBinary(tmp, dest) {
  try {
    fs.createReadStream(tmp).pipe(fs.createWriteStream(dest));

    console.log('Binary installed at', sass.getBinaryPath());
  } catch (err) {
    console.err('Cannot install binary', err.message);
  }
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
