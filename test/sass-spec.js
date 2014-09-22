var assert = require('assert'),
    fs = require('fs'),
    path = require('path'),
    sass = require('../sass');

var normalize = function(text) {
  return text.replace(/\s+/g, '').replace('{', '{\n').replace(';', ';\n');
};

describe('sass-spec', function() {
  var sassSpecPath = path.join(__dirname, 'sass-spec'),
      sassSpecDirExists = fs.existsSync(sassSpecPath);

  describe('test directory', function() {
    it('it should exist', function() {
      assert.ok(sassSpecDirExists);
    });
  });

  if (sassSpecDirExists) {
    var suitesPath = path.join(sassSpecPath, 'spec');
    var suites = fs.readdirSync(suitesPath);
    var ignoreSuites = ['todo', 'benchmarks'];

    suites.forEach(function(suite) {
      if (ignoreSuites.indexOf(suite) !== -1) {
        return;
      }

      describe(suite, function() {
        var suitePath = path.join(suitesPath, suite);
        var tests = fs.readdirSync(suitePath);

        tests.forEach(function(test) {
          var testPath = path.join(suitePath, test);

          it(test, function(done) {
            sass.render({
              file: path.join(testPath, 'input.scss'),
              includePaths: [testPath, path.join(testPath, 'sub')],
              success: function(css) {
                var expected = fs.readFileSync(path.join(testPath, 'expected_output.css'), 'utf-8');

                assert.equal(normalize(css), normalize(expected));
                done();
              },
              error: function(error) {
                done(new Error(error));
              }
            });
          });
        });
      });
    });
  }
});
