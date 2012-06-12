var binding;
var fs = require('fs');
try {
  if (fs.realpathSync(__dirname + '/build')) {
    // use the build version if it exists
    binding = require(__dirname + '/build/Release/binding');
  }
} catch (e) {
  // default to a precompiled binary if no build exists
  binding = require(__dirname + '/precompiled/'+process.platform+'/binding');
}
if (binding === null) {
  throw new Error('Cannot find appropriate binary library for node-sass');
}

exports.render = binding.render
exports.middleware = require('./lib/middleware');
