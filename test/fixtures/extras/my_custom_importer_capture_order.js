'use strict';

var path = require('path');
var existsSync = require('fs').existsSync;
var allowedExtnames = ['.scss', '.sass'];

var generate_importer = function (orderCapturingArr) {
  return function (url, prev, done) {
    var filename, dirname, fqn, result,
        isAsync = ('function' === typeof done);
    orderCapturingArr.push(url);
    filename = url;
    dirname = path.dirname(prev);
    if (-1 === allowedExtnames.indexOf(path.extname(filename))) {
      filename = filename + allowedExtnames[0];
    }
    fqn = path.resolve(dirname, filename);
    if (!existsSync(fqn)) {
      fqn = path.resolve(dirname, '_' + filename);
      if (!existsSync(fqn)) {
        result = new Error('File not found: ' + url + ' at ' + prev);
        if (isAsync) {
          done(result);
        }
        return result;
      }
    }
    result = {
      file: fqn
    };
    if (isAsync) {
      done(result);
    }
    return result;
  };
};

module.exports = generate_importer;
