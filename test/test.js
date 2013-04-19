var sass = require('../sass');
var assert = require('assert');


var scssStr = '#navbar {\
  width: 80%;\
  height: 23px; }\
  #navbar ul {\
    list-style-type: none; }\
  #navbar li {\
    float: left;\
    a {\
      font-weight: bold; }}';

// Note that the bad
var badInput = '#navbar \n\
  width: 80%';

var expectedRender = '#navbar {\n\
  width: 80%;\n\
  height: 23px; }\n\
\n\
#navbar ul {\n\
  list-style-type: none; }\n\
\n\
#navbar li {\n\
  float: left; }\n\
  #navbar li a {\n\
    font-weight: bold; }\n';


describe("compile scss", function() {
  it("should compile", function(done) {
    sass.render(scssStr, function(err, css) {
      done(err);
    });
  });

  it("should match compiled string", function(done) {
    sass.render(scssStr, function(err, css) {
      if (!err) {
        done(assert.equal(css, expectedRender));
      } else {
        done(err);
      }
    });
  });

  it("should execute asynchronously", function(done) {
    var inside, outside;
    inside = outside = false;

    sass.render(scssStr, function (err, css) {
      inside = Date.now();
    });
    outside = Date.now();

    done(assert.ok(!inside) && assert.notEqual(outside, false));
  });

  it("should execute synchronously", function(done) {
    var output = sass.renderSync(scssStr);

    done(assert.ok(output));
  });

  it("should throw an exception for bad input", function(done) {
    done(assert.throws(function() {
      sass.renderSync(badInput);
    }));
  });
});
