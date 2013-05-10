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

var badSampleFilename = 'sample.scss';
var sampleFilename = require('path').resolve(__dirname, 'sample.scss');


describe("DEPRECATED: compile scss", function() {
  it("should compile with render", function(done) {
    sass.render(scssStr, function(err, css) {
      done(err);
    });
  });

  it("should compile with renderSync", function(done) {
    done(assert.ok(sass.renderSync(scssStr)));
  });

  it("should match compiled string with render", function(done) {
    sass.render(scssStr, function(err, css) {
      if (!err) {
        done(assert.equal(css, expectedRender));
      } else {
        done(err);
      }
    });
  });

  it("should match compiled string with renderSync", function(done) {
    done(assert.equal(sass.renderSync(scssStr), expectedRender));
  });

  it("should throw an exception for bad input", function(done) {
    done(assert.throws(function() {
      sass.renderSync(badInput);
    }));
  });
});

describe("compile scss", function() {
  it("should compile with render", function(done) {
    sass.render({
      data: scssStr,
      success: function(css) {
        done(assert.ok(css));
      }
    });
  });

  it("should compile with renderSync", function(done) {
    done(assert.ok(sass.renderSync({data: scssStr})));
  });

  it("should match compiled string with render", function(done) {
    sass.render({
      data: scssStr,
      success: function(css) {
        done(assert.equal(css, expectedRender));
      },
      error: function(error) {
        done(error);
      }
    });
  });

  it("should match compiled string with renderSync", function(done) {
    done(assert.equal(sass.renderSync({data: scssStr}), expectedRender));
  });

  it("should throw an exception for bad input", function(done) {
    done(assert.throws(function() {
      sass.renderSync({data: badInput});
    }));
  });
});

describe("compile file", function() {
  it("should compile with render", function(done) {
    sass.render({
      file: sampleFilename,
      success: function (css) {
        done(assert.ok(css));
      },
      error: function (error) {
        done(error);
      }
    });
  });

    it("should compile with renderSync", function(done) {
    done(assert.ok(sass.renderSync({file: sampleFilename})));
  });

  it("should match compiled string with render", function(done) {
    sass.render({
      file: sampleFilename,
      success: function(css) {
        done(assert.equal(css, expectedRender));
      },
      error: function(error) {
        done(error);
      }
    });
  });

  it("should match compiled string with renderSync", function(done) {
    done(assert.equal(sass.renderSync({file: sampleFilename}), expectedRender));
  });

  it("should throw an exception for bad input", function(done) {
    done(assert.throws(function() {
      sass.renderSync({file: badSampleFilename});
    }));
  });
});
