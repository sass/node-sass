var assert = require('assert'),
    fs = require('fs'),
    exists = fs.existsSync,
    path = require('path'),
    read = fs.readFileSync,
    sass = require('../lib'),
    util = require('./util');

describe('spec', function () {
  var suites = util.getSuites();

  describe('test/sass-spec directory', function() {
    it('should be a cloned into place', function(done) {
      fs.exists(path.join(__dirname, 'fixtures', 'spec'), function (exists) {
        if (!exists) {
          throw new Error([
            'test/fixtures/spec directory missing. Please clone it into place by',
            'executing `git submodule update --init --recursive test/fixtures/spec`',
            'from the project\'s root directory.'
          ].join(' '));
        }

        assert(exists);
        done();
      });
    });
  });

  Object.keys(suites).forEach(function(suite) {
    var tests = Object.keys(suites[suite]);

    describe(suite, function () {
      tests.forEach(function(test) {
        var t = suites[suite][test];

        if (exists(t.src)) {
          it(test, function(done) {
            var expected = util.normalize(read(t.expected, 'utf8'));

            sass.render({
              file: t.src,
              includePaths: t.paths,
              success: function(css) {
                assert.equal(util.normalize(css), expected);
                done();
              },
              error: function(err) {
                assert(!err);
                done();
              }
            });
          });
        }
      });
    });
  });
});
