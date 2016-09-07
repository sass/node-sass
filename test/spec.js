var assert = require('assert'),
  fs = require('fs'),
  exists = fs.existsSync,
  path = require('path'),
  read = fs.readFileSync,
  sass = process.env.NODESASS_COV
      ? require('../lib-cov')
      : require('../lib'),
  util = require('./util')
  spec = require('sass-spec').dirname;

describe('spec', function() {
  this.timeout(0);
  var suites = util.getSuites();

  Object.keys(suites).forEach(function(suite) {
    var tests = Object.keys(suites[suite]);

    describe(suite, function() {
      tests.forEach(function(test) {
        var t = suites[suite][test];

        if (exists(t.src)) {
          it(test, function(done) {
            var expected = util.normalize(read(t.expected, 'utf8'));

            sass.render({
              file: t.src,
              includePaths: t.paths
            }, function(error, result) {
              if (t.error) {
                assert(error);
              } else {
                assert(!error);
              }
              if (expected) {
                assert.equal(util.normalize(result.css.toString()), expected);
              }
              done();
            });
          });
        }
      });
    });
  });
});
