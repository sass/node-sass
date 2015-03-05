/*!
 * node-sass: scripts/upload.js
 */
require('../lib/extensions');

var flags = require('meow')({ pkg: '../' }).flags;

var fetchReleaseInfoUrl = ['https://api.github.com/repos/sass/node-sass/releases/tags/v',
                           flags.tag ? flags.tag : require('../package.json').version].join(''),
    file = flags.path ?
             flags.path :
             require('path').resolve(__dirname, '..', 'vendor', process.sassBinaryName, 'binding.node'),
    fs = require('fs'),
    os = require('os'),
    request = require('request'),
    uploadReleaseAssetUrl = ['?name=', process.sassBinaryName, '.node', '&label=', process.sassBinaryName].join('');

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
      }

      console.log(['Binary uploaded successfully.',
                   'Please test the following link before announcement it:',
                   formattedResponse.browser_download_url].join(os.EOL));
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
    'The server returned:', JSON.stringify(err)].join(os.EOL));
}

/**
 * Run
 */

console.log('Preparing to uploading', file, '..');
uploadBinary();
