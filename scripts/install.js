/*!
 * node-sass: scripts/install.js
 */

var fs = require('fs'),
  eol = require('os').EOL,
  path = require('path'),
  fetch = require('make-fetch-happen'),
  sass = require('../lib/extensions'),
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
    cb(['Cannot download "', url, '": ', eol, eol,
      typeof err.message === 'string' ? err.message : err, eol, eol,
      'Hint: If github.com is not accessible in your location', eol,
      '      try setting a proxy via HTTP_PROXY, e.g. ', eol, eol,
      '      export HTTP_PROXY=http://example.com:1234',eol, eol,
      'or configure npm proxy via', eol, eol,
      '      npm config set proxy http://example.com:8080'].join(''));
  };

  var successful = function(response) {
    return response.status >= 200 && response.status < 300;
  };

  console.log('Downloading binary from', url);

  try {
    fetch(url, downloadOptions()).then(function (response) {
      return response.buffer();
    }).then(function (buffer) {
      fs.createWriteStream(dest).on('error', cb).end(buffer, cb);
      console.log('Download complete');
    }).catch(function(err) {
      if(!successful(err)) {
        reportError(['HTTP error', err.code, err.message].join(' '));
      } else {
        reportError(err);
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
  if (process.env.SKIP_SASS_BINARY_DOWNLOAD_FOR_CI) {
    console.log('Skipping downloading binaries on CI builds');
    return;
  }

  var cachedBinary = sass.getCachedBinary(),
    cachePath = sass.getBinaryCachePath(),
    binaryPath = sass.getBinaryPath();

  if (sass.hasBinary(binaryPath)) {
    console.log('node-sass build', 'Binary found at', binaryPath);
    return;
  }

  try {
    fs.mkdirSync(path.dirname(binaryPath), {recursive: true});
  } catch (err) {
    console.error('Unable to save binary', path.dirname(binaryPath), ':', err);
    return;
  }

  if (cachedBinary) {
    console.log('Cached binary found at', cachedBinary);
    fs.createReadStream(cachedBinary).pipe(fs.createWriteStream(binaryPath));
    return;
  }

  download(sass.getBinaryUrl(), binaryPath, function(err) {
    if (err) {
      console.error(err);
      return;
    }

    console.log('Binary saved to', binaryPath);

    cachedBinary = path.join(cachePath, sass.getBinaryName());

    if (cachePath) {
      console.log('Caching binary to', cachedBinary);

      try {
        fs.mkdirSync(path.dirname(cachedBinary), {recursive: true});
        fs.createReadStream(binaryPath)
          .pipe(fs.createWriteStream(cachedBinary))
          .on('error', function (err) {
            console.log('Failed to cache binary:', err);
          });
      } catch (err) {
        console.log('Failed to cache binary:', err);
      }
    }
  });
}

/**
 * If binary does not exist, download it
 */

checkAndDownloadBinary();
