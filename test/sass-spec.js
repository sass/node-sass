var path = require('path'),
  assert = require('assert'),
  fs = require('fs'),
  os = require('os'),
  sassSpecPath = path.join(__dirname, '../sass-spec'),
  sass = require('../sass');

// Check for the required clone of the sass-spec repo under the root
fs.exists(sassSpecPath, function(exists) {
  if (exists) {
    console.log('found sass-spec path!');
  } else {
    console.log('please run "git clone https://github.com/hcatlin/sass-spec.git" from the root of the project!');
    process.exit(1);
  }
});

var suites = fs.readdirSync(path.join(sassSpecPath, 'spec'));

suites = suites.filter(function(element){
  return !(element === 'todo' || element === 'benchmarks' || element === 'basic-extend-tests' || element === 'extend-tests');
});

// Each subfolder of spec is a separate suite of Sass tests
suites.forEach(function (suiteDir) {

  describe(suiteDir, function() {
    var tests = fs.readdirSync(path.join(sassSpecPath, 'spec', suiteDir));

    // Each subfolder of the suite folder contains a named folder with an expected
    // 'input.scss' and the expected 'expected_output.css' that it should compile to
    tests.forEach(function (testDir) {
      it(testDir, function(done) {
        var input = path.join(sassSpecPath, 'spec', suiteDir, testDir, 'input.scss');
        sass.render({
          file: input,
          success: function (css) {
            var expected_output = fs.readFileSync(path.join(sassSpecPath, 'spec', suiteDir, testDir, 'expected_output.css'), 'utf-8');

            if (os.platform() === 'win32') {
              expected_output = expected_output.replace(/\r/g, '');
            }

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

});
