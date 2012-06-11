var binding = require('./build/Release/binding')

exports.render = binding.render
exports.middleware = require('./lib/middleware');
