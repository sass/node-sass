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

  describe('spec directory', function() {
    it('should be a cloned into place', function() {
      try {
        assert.ok(sassSpecDirExists);
      } catch (e) {
        console.log([
          'test/sass-spec directory missing. Please clone it into place by',
          'executing `git submodule update --init --recursive test/sass-spec`',
          'from the project\'s root directory.'
        ].join(' '));

        throw e;
      }
    });
  });

  if (sassSpecDirExists) {
    var suitesPath = path.join(sassSpecPath, 'spec');
    var suites = fs.readdirSync(suitesPath);
    var ignoreSuites = ['libsass-todo-issues', 'libsass-todo-tests'];

    suites.forEach(function(suite) {
      if (ignoreSuites.indexOf(suite) !== -1) {
        return;
      }

      describe(suite, function() {
        var suitePath = path.join(suitesPath, suite);
        var tests = fs.readdirSync(suitePath);

        tests.forEach(function(test) {
          var testPath = path.join(suitePath, test);
          var inputFilePath = path.join(testPath, 'input.scss');

          if (fs.existsSync(inputFilePath)) {
            it(test, function(done) {
              sass.render({
                file: inputFilePath,
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
          } else {
            it(test);
          }
        });
      });
    });
  }
});
