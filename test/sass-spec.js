var path = require('path'),
  assert = require('assert'),
  fs = require('fs'),
  sassSpecPath = path.join(__dirname, 'sass-spec'),
  sass = require('../sass');

// Check for the required clone of the sass-spec repo under the root
fs.exists(sassSpecPath, function(exists) {
  describe('sass-spec', function() {
    if (exists) {
      var suites = fs.readdirSync(path.join(sassSpecPath, 'spec'));

      // Each subfolder of spec is a separate suite of Sass tests
      suites.forEach(function (suiteDir) {

        if (suiteDir === 'todo' || suiteDir === 'benchmarks' || suiteDir === 'basic-extend-tests' || suiteDir === 'extend-tests') {
          describe.skip(suiteDir, function() {});
        } else {
          describe(suiteDir, function() {
            var tests = fs.readdirSync(path.join(sassSpecPath, 'spec', suiteDir));

            // Each subfolder of the suite folder contains a named folder with an expected
            // 'input.scss' and the expected 'expected_output.css' that it should compile to
            tests.forEach(function (testDir) {
              it(testDir, function(done) {
                var testPath = path.join(sassSpecPath, 'spec', suiteDir, testDir, '/');
                var input = path.join(sassSpecPath, 'spec', suiteDir, testDir, 'input.scss');
                sass.render({
                  file: input,
                  includePaths: [testPath, path.join(testPath, 'sub/')],
                  success: function (css) {
                    var expected_output = fs.readFileSync(path.join(sassSpecPath, 'spec', suiteDir, testDir, 'expected_output.css'), 'utf-8');

                    expected_output = expected_output.replace(/(\r\n|\r)/g, '\n').replace(/\n$/g, '');
                    css = css.replace(/\n$/g, '');

                    assert.equal(css, expected_output);
                    done();
                  },
                  error: function (error) {
                    done(error);
                  }
                });
              });
            });
          });
        }

      });
    } else {
      it.skip('please run "git clone https://github.com/hcatlin/sass-spec.git" from the root of the project!');
    }
  });
});
