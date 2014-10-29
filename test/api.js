var assert = require('assert'),
    fs = require('fs'),
    path = require('path'),
    read = fs.readFileSync,
    sass = require('../sass'),
    fixture = path.join.bind(null, __dirname, 'fixtures');

describe('api (deprecated)', function() {
  describe('.render(src, fn)', function() {
    it('should compile sass to css', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8');

      sass.render(src, function(err, css) {
        assert(!err);
        assert.equal(css, expected);
        done();
      });
    });

    it('should throw error for bad input', function(done) {
      sass.render('#navbar width 80%;', function(err) {
        assert(err);
        done();
      });
    });
  });

  describe('.renderSync(src)', function() {
    it('should compile sass to css', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8');

      assert.equal(sass.renderSync(src), expected);
      done();
    });

    it('should throw error for bad input', function(done) {
      assert.throws(function() {
        sass.renderSync('#navbar width 80%;');
      });

      done();
    });
  });
});

describe('api', function() {
  describe('.render(options)', function() {
    it('should compile sass to css', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8');

      sass.render({
        data: src,
        success: function(css) {
          assert.equal(css, expected);
          done();
        }
      });
    });

    it('should throw error status 1 for bad input', function(done) {
      sass.render({
        data: '#navbar width 80%;',
        error: function(err, status) {
          assert(err);
          assert.equal(status, 1);
          done();
        }
      });
    });

    it('should compile with include paths', function(done) {
      var src = read(fixture('include-path/index.scss'), 'utf8');
      var expected = read(fixture('include-path/expected.css'), 'utf8');

      sass.render({
        data: src,
        includePaths: [
          fixture('include-path/functions'),
          fixture('include-path/lib')
        ],
        success: function(css) {
          assert.equal(css, expected);
          done();
        }
      });
    });

    it('should compile with include paths', function(done) {
      var src = read(fixture('include-path/index.scss'), 'utf8');
      var expected = read(fixture('include-path/expected.css'), 'utf8');

      sass.render({
        data: src,
        includePaths: [
          fixture('include-path/functions'),
          fixture('include-path/lib')
        ],
        success: function(css) {
          assert.equal(css, expected);
          done();
        }
      });
    });

    it('should compile with image path', function(done) {
      var src = read(fixture('image-path/index.scss'), 'utf8');
      var expected = read(fixture('image-path/expected.css'), 'utf8');

      sass.render({
        data: src,
        imagePath: '/path/to/images',
        success: function(css) {
          assert.equal(css, expected);
          done();
        }
      });
    });

    it('should throw error with non-string image path', function(done) {
      var src = read(fixture('image-path/index.scss'), 'utf8');

      assert.throws(function() {
        sass.render({
          data: src,
          imagePath: ['/path/to/images']
        });
      });

      done();
    });

    it('should render with --precision option', function(done) {
      var src = read(fixture('precision/index.scss'), 'utf8');
      var expected = read(fixture('precision/expected.css'), 'utf8');

      sass.render({
        data: src,
        precision: 10,
        success: function(css) {
          assert.equal(css, expected);
          done();
        }
      });
    });

    it('should compile with stats', function(done) {
      var src = fixture('precision/index.scss');
      var stats = {};

      sass.render({
        file: src,
        stats: stats,
        sourceMap: true,
        success: function() {
          assert.equal(stats.entry, src);
          done();
        }
      });
    });
  });

  describe('.renderSync(options)', function() {
    it('should compile with renderSync', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8');

      assert.equal(sass.renderSync({ data: src }), expected);
      done();
    });

    it('should throw error for bad input', function(done) {
      assert.throws(function() {
        sass.renderSync({ data: '#navbar width 80%;' });
      });

      done();
    });
  });

  describe('.renderFile(options)', function() {
    it('should compile with renderFile', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var dest = fixture('simple/build.css');
      var expected = read(fixture('simple/expected.css'), 'utf8');

      sass.renderFile({
        data: src,
        outFile: dest,
        success: function() {
          assert.equal(read(dest, 'utf8'), expected);
          done();
        }
      });
    });

    it('should save source map to default name', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/build.css');
      var name = 'build.css.map';

      sass.renderFile({
        file: src,
        outFile: dest,
        sourceMap: true,
        success: function(file, map) {
          assert.equal(path.basename(map), name);
          assert(read(dest, 'utf8').indexOf('sourceMappingURL=' + name) !== -1);
          fs.unlinkSync(map);
          fs.unlinkSync(dest);
          done();
        }
      });
    });

    it('should save source map to specified name', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/build.css');
      var name = 'foo.css.map';

      sass.renderFile({
        file: src,
        outFile: dest,
        sourceMap: name,
        success: function(file, map) {
          assert.equal(path.basename(map), name);
          assert(read(dest, 'utf8').indexOf('sourceMappingURL=' + name) !== -1);
          fs.unlinkSync(map);
          fs.unlinkSync(dest);
          done();
        }
      });
    });

    it('should save source paths relative to the source map file', function(done) {
      var src = fixture('include-files/index.scss');
      var dest = fixture('include-files/build.css');
      var obj;

      sass.renderFile({
        file: src,
        outFile: dest,
        sourceMap: true,
        success: function(file, map) {
          obj = JSON.parse(read(map, 'utf8'));
          assert.equal(obj.sources[0], 'index.scss');
          assert.equal(obj.sources[1], 'foo.scss');
          assert.equal(obj.sources[2], 'bar.scss');
          fs.unlinkSync(map);
          fs.unlinkSync(dest);
          done();
        }
      });
    });
  });

  describe('.render({ stats: {} })', function() {
    var start = Date.now();
    var stats = {};

    before(function (done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        stats: stats,
        success: function () {
          done();
        },
        error: function (err) {
          assert(!err);
          done();
        }
      });
    });

    it('should provide a start timestamp', function(done) {
      assert(typeof stats.start === 'number');
      assert(stats.start >= start);
      done();
    });

    it('should provide an end timestamp', function(done) {
      assert(typeof stats.end === 'number');
      assert(stats.end >= stats.start);
      done();
    });

    it('should provide a duration', function(done) {
      assert(typeof stats.duration === 'number');
      assert.equal(stats.end - stats.start, stats.duration);
      done();
    });

    it('should contain the given entry file', function(done) {
      assert.equal(stats.entry, fixture('include-files/index.scss'));
      done();
    });

    it('should contain an array of all included files', function(done) {
      assert.equal(stats.includedFiles[0], fixture('include-files/bar.scss'));
      assert.equal(stats.includedFiles[1], fixture('include-files/foo.scss'));
      assert.equal(stats.includedFiles[2], fixture('include-files/index.scss'));
      done();
    });

    it('should contain array with the entry if there are no import statements', function(done) {
      sass.render({
        file: fixture('simple/index.scss'),
        stats: stats,
        success: function () {
          assert.deepEqual(stats.includedFiles, [fixture('simple/index.scss')]);
          done();
        }
      });
    });

    it('should state `data` as entry file', function(done) {
      sass.render({
        data: read(fixture('simple/index.scss'), 'utf8'),
        stats: stats,
        success: function () {
          assert.equal(stats.entry, 'data');
          done();
        }
      });
    });

    it('should contain an empty array as includedFiles', function(done) {
      sass.render({
        data: read(fixture('simple/index.scss'), 'utf8'),
        stats: stats,
        success: function () {
          assert.deepEqual(stats.includedFiles, []);
          done();
        }
      });
    });
  });

  describe('.renderSync({ stats: {} })', function() {
    var start = Date.now();
    var stats = {};

    before(function () {
      sass.renderSync({
        file: fixture('include-files/index.scss'),
        stats: stats
      });
    });

    it('should provide a start timestamp', function(done) {
      assert(typeof stats.start === 'number');
      assert(stats.start >= start);
      done();
    });

    it('should provide an end timestamp', function(done) {
      assert(typeof stats.end === 'number');
      assert(stats.end >= stats.start);
      done();
    });

    it('should provide a duration', function(done) {
      assert(typeof stats.duration === 'number');
      assert.equal(stats.end - stats.start, stats.duration);
      done();
    });

    it('should contain the given entry file', function(done) {
      assert.equal(stats.entry, fixture('include-files/index.scss'));
      done();
    });

    it('should contain an array of all included files', function(done) {
      assert.equal(stats.includedFiles[0], fixture('include-files/bar.scss'));
      assert.equal(stats.includedFiles[1], fixture('include-files/foo.scss'));
      assert.equal(stats.includedFiles[2], fixture('include-files/index.scss'));
      done();
    });

    it('should contain array with the entry if there are no import statements', function(done) {
      sass.renderSync({
        file: fixture('simple/index.scss'),
        stats: stats
      });

      assert.deepEqual(stats.includedFiles, [fixture('simple/index.scss')]);
      done();
    });

    it('should state `data` as entry file', function(done) {
      sass.renderSync({
        data: read(fixture('simple/index.scss'), 'utf8'),
        stats: stats
      });

      assert.equal(stats.entry, 'data');
      done();
    });

    it('should contain an empty array as includedFiles', function(done) {
      sass.renderSync({
        data: read(fixture('simple/index.scss'), 'utf8'),
        stats: stats
      });

      assert.deepEqual(stats.includedFiles, []);
      done();
    });
  });
});
