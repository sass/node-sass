var assert = require('assert'),
    path = require('path'),
    read = require('fs').readFileSync,
    fixture = path.join.bind(null, __dirname, 'fixtures'),
    sass = process.env.NODESASS_COV ? require('../lib-cov') : require('../lib');

describe('importer order', function() {
  it('should callback with imported node by DFS order', function (done) {
    var src = fixture('depth-first/index.scss');
    var expected = read(fixture('depth-first/expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
    var expectedOrder = require(fixture('depth-first/expectedOrder.js'));
    var capturedOrder = [];
    var orderCaptureImporter = require(fixture('extras/my_custom_importer_capture_order.js'))(capturedOrder);
    var sassOptions = {
      file: src,
      importer: orderCaptureImporter
    };

    sass.render(sassOptions, function(err, result) {
      var actual;
      if (err) {
        done(err);
        return;
      }
      actual = result.css.toString();
      assert.strictEqual(actual.trim().replace(/\r\n/g, '\n'), expected);
      assert.deepEqual(capturedOrder, expectedOrder);
      done();
    });
  });
});

