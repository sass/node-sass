var fs = require('fs'),
    path = require('path'),
    Download = require('download'),
    status = require('download-status');

/**
 * Check if binaries exists
 *
 * @api private
 */

function exists() {
  var v8 = 'v8-' + /[0-9]+\.[0-9]+/.exec(process.versions.v8)[0];
  var name = process.platform + '-' + process.arch + '-' + v8;

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
  var download = new Download({
    extract: true,
    mode: '777',
    strip: 1
  });

  var url = [
    'https://raw.githubusercontent.com/sass/node-sass/v',
    require('../package.json').version, '/', name,
    '/binding.node'
  ].join('');

  download.get(url);
  download.dest(path.join(__dirname, '..', 'vendor', name));
  download.use(status());

  download.run(function(err) {
    if (err) {
      console.error(err.message);
      return;
    }

    console.log('Binary installed in ' + download.dest());
  });
}

/**
 * Skip if CI
 */

if (process.env.CI || process.env.APPVEYOR) {
  console.log('Skipping downloading binaries on CI builds');
  return;
}

/**
 * Run
 */

exists();
