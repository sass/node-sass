var binding = require('./build/Release/binding')

var render = function(str, cb){
  cb(binding.render(str))
}

exports.render = render
exports.middleware = require('./lib/middleware');