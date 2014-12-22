var path = require('path');

module.exports = function(file) {console.log('>>>>>>>>>>');console.log(path.resolve(path.join(process.cwd(), 'test/fixtures/include-files/', file + (path.extname(file) ? '' : '.scss'))));
  return {
      file: path.resolve(path.join(process.cwd(), 'test/fixtures/include-files/', file + (path.extname(file) ? '' : '.scss')))
  };
};
