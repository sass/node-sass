/*!
 * node-sass: scripts/upload.js
 */

require('../lib/extensions');

var flags = require('meow')({ pkg: '../' }).flags;
var eol = require('os').EOL,
    fetchReleaseInfoUrl = ['https://api.github.com/repos/sass/node-sass/releases/tags/v',
                           flags.tag ? flags.tag : require('../package.json').version].join(''),
    file = flags.path ? flags.path : process.sass.binaryPath,
    fs = require('fs'),
    request = require('request'),
    uploadReleaseAssetUrl = ['?name=', process.sass.binaryName].join('');

/**
 * Upload binary using GitHub API
 *
 * @api private
 */

function uploadBinary() {
  if (!fs.existsSync(file)) {
    throw new Error('Error reading file.');
  }

  var post = function() {
    request.post({
      url: uploadReleaseAssetUrl,
      headers: {
        'Authorization': ['Token ', flags.auth].join(''),
        'Content-Type': 'application/octet-stream'
      },
      formData: {
        file: fs.createReadStream(file)
      }
    }, function(err, res, body) {
      if (err) {
        throwFormattedError(err);
      }

      var formattedResponse = JSON.parse(body);

      if (formattedResponse.errors) {
        throwFormattedError(formattedResponse.errors);
      } else if (res.statusCode > 399) {
        throwFormattedError(formattedResponse);
      }

      console.log(['Binary uploaded successfully.',
                   'Please test the following link before announcing it:',
                   formattedResponse.browser_download_url].join(eol));
    });
  };

  request.get({
    url: fetchReleaseInfoUrl,
    headers: {
      'User-Agent': 'Node-Sass-Release-Agent'
    }
  }, function(err, res, body) {
    if (err) {
      throw new Error('Error fetching release id.');
    }

    var upload_url = JSON.parse(body).upload_url;
    uploadReleaseAssetUrl = upload_url.replace(/{\?name}/, uploadReleaseAssetUrl);

    console.log('Upload URL is:', uploadReleaseAssetUrl);

    post();
  });
}

function throwFormattedError(err) {
  throw new Error([
    'Error uploading release asset.',
    'The server returned:', JSON.stringify(err)].join(eol));
}

/**
 * Run
 */

console.log('Preparing to uploading', file, '..');
uploadBinary();
