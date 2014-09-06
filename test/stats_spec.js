'use strict';

var path = require('path');
var assert = require('assert');
var sass = process.env.NODESASS_COVERAGE ? require('../sass-coverage') : require('../sass');
var includedFilesFile = path.resolve(__dirname, 'included_files.scss').replace(/\\/g, '/');
var sampleFile = path.resolve(__dirname, 'sample.scss').replace(/\\/g, '/');
var imagePathFile = path.resolve(__dirname, 'image_path.scss').replace(/\\/g, '/');
var sample = require('./sample.js');

describe('stats', function() {
  var start = Date.now();
  var stats;

  function checkTimingStats() {
    it('should provide a start timestamp', function() {
      assert.ok(typeof stats.start === 'number');
      assert.ok(stats.start >= start);
    });

    it('should provide an end timestamp', function() {
      assert.ok(typeof stats.end === 'number');
      assert.ok(stats.end >= stats.start);
    });

    it('should provide a duration', function() {
      assert.ok(typeof stats.duration === 'number');
      assert.equal(stats.end - stats.start, stats.duration);
    });
  }

  describe('using renderSync()', function() {

    describe('and file-context', function() {

      before(function() {
        sass.renderSync({
          file: includedFilesFile,
          stats: stats = {}
        });
      });

      checkTimingStats();

      it('should contain the given entry file', function() {
        assert.equal(stats.entry, includedFilesFile);
      });

      it('should contain an array of all included files', function() {
        // the included_files aren't sorted by libsass in any way
        assert.deepEqual(
          stats.includedFiles.sort(),
          [includedFilesFile, sampleFile, imagePathFile].sort()
        );
      });

      it('should contain an array with the entry-file if the there are no import statements', function () {
        sass.renderSync({
          file: sampleFile,
          stats: stats = {}
        });
        assert.deepEqual(stats.includedFiles, [sampleFile]);
      });

    });

    describe('and data-context', function() {

      before(function() {
        sass.renderSync({
          data: sample.input,
          stats: stats = {}
        });
      });

      checkTimingStats();

      it('should state "data" as entry file', function() {
        assert.equal(stats.entry, 'data');
      });

      it('should contain an empty array as includedFiles in the data-context', function() {
        assert.deepEqual(stats.includedFiles, []);
      });

    });

  });

  describe('using render()', function () {

    describe('and file-context', function() {

      before(function(done) {
        sass.render({
          file: includedFilesFile,
          stats: stats = {},
          success: function() {
            done();
          },
          error: done
        });
      });

      checkTimingStats();

      it('should contain the given entry file', function() {
        assert.equal(stats.entry, includedFilesFile);
      });

      it('should contain an array of all included files', function() {
        // the included_files aren't sorted by libsass in any way
        assert.deepEqual(
          stats.includedFiles.sort(),
          [includedFilesFile, sampleFile, imagePathFile].sort()
        );
      });

      it('should contain an array with the entry-file if the there are no import statements', function(done) {
        sass.render({
          file: sampleFile,
          stats: stats = {},
          success: function() {
            assert.deepEqual(stats.includedFiles, [sampleFile]);
            done();
          },
          error: done
        });
      });

    });

    describe('and data-context', function() {

      before(function(done) {
        sass.render({
          data: sample.input,
          stats: stats = {},
          success: function() {
            done();
          },
          error: done
        });
      });

      checkTimingStats();

      it('should state "data" as entry file', function() {
        assert.equal(stats.entry, 'data');
      });

      it('should contain an empty array as includedFiles in the data-context', function() {
        assert.deepEqual(stats.includedFiles, []);
      });

    });

  });

});
