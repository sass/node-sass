var fs = require('fs');
var sass = require('../sass');
var assert = require('assert');

describe("compiling input with mixins", function() {
  var input = fs.readFileSync('test/assets/mixins.scss', 'utf8');
  var expectedRender = fs.readFileSync('test/assets/mixins_expected_output.css', 'utf8');

  it("should compile", function(done) {
    sass.render(input, function(err, css) {
      if (err) { done(err) }
      assert.equal(css, expectedRender);
      done();
    });
  });
});
