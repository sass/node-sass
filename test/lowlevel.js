process.env.NODESASS_COV ? require('../lib-cov') : require('../lib');

var assert = require('assert'),
    binding = require(process.sass.binaryPath);

describe('lowlevel', function() {
  it('fail with options not an object', function(done) {
    var options =  2;
    assert.throws(function() {
      binding.renderSync(options);
    }, /"result" element is not an object/);
    done();
  });

  it('fail with options.result not provided', function(done) {
    var options =  { data: 'div { width: 10px; } ',
      sourceComments: false,
      file: null,
      outFile: null,
      includePaths: '',
      precision: 5,
      sourceMap: null,
      style: 0,
      indentWidth: 2,
      indentType: 0,
      linefeed: '\n' };

    assert.throws(function() {
      binding.renderSync(options);
    }, /"result" element is not an object/);
    done();
  });


  it('fail with options.result not an object', function(done) {
    var options =  { data: 'div { width: 10px; } ',
      sourceComments: false,
      file: null,
      outFile: null,
      includePaths: '',
      precision: 5,
      sourceMap: null,
      style: 0,
      indentWidth: 2,
      indentType: 0,
      linefeed: '\n',
      result: 2 };

    assert.throws(function() {
      binding.renderSync(options);
    }, /"result" element is not an object/);
    done();
  });


  it('fail with options.result.stats not provided', function(done) {

    var options =  { data: 'div { width: 10px; } ',
      sourceComments: false,
      file: null,
      outFile: null,
      includePaths: '',
      precision: 5,
      sourceMap: null,
      style: 0,
      indentWidth: 2,
      indentType: 0,
      linefeed: '\n',
      result: {} };

    assert.throws(function() {
      binding.renderSync(options);
    }, /"result.stats" element is not an object/);
    done();
  });

  it('fail with options.result.stats not an object', function(done) {

    var options =  { data: 'div { width: 10px; } ',
      sourceComments: false,
      file: null,
      outFile: null,
      includePaths: '',
      precision: 5,
      sourceMap: null,
      style: 0,
      indentWidth: 2,
      indentType: 0,
      linefeed: '\n',
      result: { stats: 2 } };

    assert.throws(function() {
      binding.renderSync(options);
    }, /"result.stats" element is not an object/);
    done();
  });

  it('options.indentWidth not provided', function(done) {
    var options =  { data: 'div { width: 10px; } ',
      sourceComments: false,
      file: null,
      outFile: null,
      includePaths: '',
      precision: 5,
      sourceMap: null,
      style: 0,
      /* indentWidth */
      indentType: 0,
      linefeed: '\n',
      result: { stats: {} } };

      binding.renderSync(options);
    done();
  });


}); // lowlevel
