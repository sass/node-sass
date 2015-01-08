var fs = require('fs'),
    path = require('path'),
    request = require('request'),
    mkdirp = require('mkdirp'),
    exec = require('shelljs').exec;

/**
 * Download file, if succeeds save, if not delete
 *
 * @param {String} url
 * @param {String} dest
 * @param {function} cb
 * @api private
 */

function download(url, dest, cb) {
  var file = fs.createWriteStream(dest);
  var options = { proxy: getProxy(), rejectUnauthorized: false };
  var returnError = function(err) {
    fs.unlink(dest);
    cb(typeof err.message === 'string' ? err.message : err);
  };
  var req = request.get(url, options).on('response', function(response) {
    if (response.statusCode < 200 || response.statusCode >= 300) {
      returnError('Can not download file from ' + url);
      return;
    }
    response.pipe(file);

    file.on('finish', function() {
        file.close(cb);
    });
  }).on('error', returnError);

  req.end();
  req.on('error', returnError);
}

/**
 * Get proxy settings
 *
 * @api private
 */

function getProxy() {
  var result = ['https-proxy', 'proxy', 'http-proxy'].filter(function(config) {
    var proxy = exec('npm config get ' + config, {silent: true});

    return proxy.code === 0 && validateProxyUrl(proxy.output.trim());
  })[0];

  if (result) {
    return result;
  }

  var env = process.env;
  return env.HTTPS_PROXY || env.https_proxy || env.HTTP_PROXY || env.http_proxy;
}

/**
 * Validates Proxy URL
 *
 * @param {String} url
 * @api private
 */

function validateProxyUrl(url) {
  if (/\n/.test(url)) {
    url = url.replace(/\r?\n+/, '\n').split('\n');
    url = url[url.length - 3].trim(); // get the second last element.
  }

  return url !== 'null' &&
         url !== 'undefined' &&
         url === require('url').parse(url);
}

/**
 * Check if binaries exists
 *
 * @api private
 */

function exists() {
  var name = process.platform + '-' + process.arch;

  fs.exists(path.join(__dirname, '..', 'vendor', name), function (exists) {
    if (exists) {
      return;
    }

    fetch(name);
  });
}

/**
 * Fetch binaries
 *
 * @param {String} name
 * @api private
 */

function fetch(name) {
  var url = [
    'https://raw.githubusercontent.com/sass/node-sass-binaries/v',
    require('../package.json').version, '/', name,
    '/binding.node'
  ].join('');
  var dir = path.join(__dirname, '..', 'vendor', name);
  var dest = path.join(dir, 'binding.node');

  mkdirp(dir, function(err) {
    if (err) {
      console.error(err);
      return;
    }

    download(url, dest, function(err) {
      if (err) {
        console.error(err);
        return;
      }

      console.log('Binary downloaded and installed at ' + dest);
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
 * Run
 */

exists();
