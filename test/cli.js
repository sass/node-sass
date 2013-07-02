var path   = require('path'),
    assert = require('assert'),
    fs     = require('fs'),
    exec   = require('child_process').exec,
    sass   = require(path.join(__dirname, '..', 'sass')),

    cliPath = path.resolve(__dirname, '..', 'bin', 'node-sass'),
    sampleFilename = path.resolve(__dirname, 'sample.scss');


describe('cli', function() {
  it('should print help when run with no arguments', function(done) {
    exec(cliPath, function(err, stdout, stderr) {
      done(assert(stderr.indexOf('Compile .scss files with node-sass') === 0));
    });
  });

  it('should compile sample.scss as sample.css', function(done) {
    var resultPath = path.join(__dirname, 'sample.css');

    exec(cliPath + ' ' + sampleFilename, {
      cwd: __dirname
    }, function(err, stdout, stderr) {

      fs.exists(resultPath, function(exists) {
        done(assert(exists));

        fs.unlink(resultPath, function() {});
      });
    });
  });

  it('should compile sample.scss to  ../out.css', function(done) {
    var resultPath = path.resolve(__dirname, '..', 'out.css');

    exec(cliPath + ' ' + sampleFilename + ' ../out.css', {
      cwd: __dirname
    }, function(err, stdout, stderr) {

      fs.exists(resultPath, function(exists) {
        done(assert(exists));

        fs.unlink(resultPath, function() {});
      });
    });
  });
});
