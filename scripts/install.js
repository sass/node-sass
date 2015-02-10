var fs = require('fs'),
    path = require('path'),
    request = require('request'),
    mkdirp = require('mkdirp'),
    exec = require('shelljs').exec,
    utils = require('../lib/utils');

/**
 * Download file, if succeeds save, if not delete
 *
 * @param {String} url
 * @param {String} dest
 * @param {Function} cb
 * @api private
 */

function download(url, dest, cb) {
  var file = fs.createWriteStream(dest);

  applyProxy({ rejectUnauthorized: false }, function(options) {
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
  require('npmconf').load({}, function (er, conf) {
    var getProxyFromEnv = true;
    ['https-proxy', 'proxy', 'http-proxy'].forEach(function(setting) {
      var proxyUrl = conf.get(setting);

      if(proxyUrl && proxyUrl === require('url').parse(proxyUrl)) {
        options.proxy = proxyUrl;
        getProxyFromEnv = false;
        cb(options);
        return;
      }
    });

    if(getProxyFromEnv) {
      var env = process.env;
      options.proxy = env.HTTPS_PROXY || env.https_proxy || env.HTTP_PROXY || env.http_proxy;

      cb(options);
    }
  });
}

/**
 * Check if binaries exists
 *
 * @api private
 */

function exists() {
  var name = utils.getBinaryIdentifiableName();

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
