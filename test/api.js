var assert = require('assert'),
    fs = require('fs'),
    path = require('path'),
    read = fs.readFileSync,
    sass = process.env.NODESASS_COV ? require('../lib-cov') : require('../lib'),
    fixture = path.join.bind(null, __dirname, 'fixtures'),
    resolveFixture = path.resolve.bind(null, __dirname, 'fixtures');

describe('api', function() {
  describe('.render(options)', function() {
    it('should compile sass to css with file', function(done) {
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();

      sass.render({
        file: fixture('simple/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
          done();
        }
      });
    });

    it('should compile sass to css with data', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();

      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
          done();
        }
      });
    });

    it('should compile sass to css using indented syntax', function(done) {
      var src = read(fixture('indent/index.sass'), 'utf8');
      var expected = read(fixture('indent/expected.css'), 'utf8').trim();

      sass.render({
        data: src,
        indentedSyntax: true,
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
          done();
        }
      });
    });

    it('should throw error status 1 for bad input', function(done) {
      sass.render({
        data: '#navbar width 80%;',
        error: function(error) {
          assert(error.message);
          assert.equal(error.status, 1);
          done();
        }
      });
    });

    it('should compile with include paths', function(done) {
      var src = read(fixture('include-path/index.scss'), 'utf8');
      var expected = read(fixture('include-path/expected.css'), 'utf8').trim();

      sass.render({
        data: src,
        includePaths: [
          fixture('include-path/functions'),
          fixture('include-path/lib')
        ],
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
          done();
        }
      });
    });

    it('should compile with image path', function(done) {
      var src = read(fixture('image-path/index.scss'), 'utf8');
      var expected = read(fixture('image-path/expected.css'), 'utf8').trim();

      sass.render({
        data: src,
        imagePath: '/path/to/images',
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
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
      var expected = read(fixture('precision/expected.css'), 'utf8').trim();

      sass.render({
        data: src,
        precision: 10,
        success: function(result) {
          assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
          done();
        }
      });
    });

    it('should contain all included files in stats when data is passed', function(done) {
      var src = read(fixture('include-files/index.scss'), 'utf8');
      var expected = [
        fixture('include-files/bar.scss').replace(/\\/g, '/'),
        fixture('include-files/foo.scss').replace(/\\/g, '/')
      ];

      sass.render({
        data: src,
        includePaths: [fixture('include-files')],
        success: function(result) {
          assert.deepEqual(result.stats.includedFiles, expected);
          done();
        }
      });
    });
  });

  describe('.render(importer)', function() {
    var src = read(fixture('include-files/index.scss'), 'utf8');

    it('should override imports with "data" as input and fires callback with file and contents', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev, done) {
          done({
            file: '/some/other/path.scss',
            contents: 'div {color: yellow;}'
          });
        }
      });
    });

    it('should override imports with "file" as input and fires callback with file and contents', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev, done) {
          done({
            file: '/some/other/path.scss',
            contents: 'div {color: yellow;}'
          });
        }
      });
    });

    it('should override imports with "data" as input and returns file and contents', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev) {
          return {
            file: prev + url,
            contents: 'div {color: yellow;}'
          };
        }
      });
    });

    it('should override imports with "file" as input and returns file and contents', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev) {
          return {
            file: prev + url,
            contents: 'div {color: yellow;}'
          };
        }
      });
    });

    it('should override imports with "data" as input and fires callback with file', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), '');
          done();
        },
        importer: function(url, /* jshint unused:false */ prev, done) {
          done({
            file: path.resolve(path.dirname(fixture('include-files/index.scss')), url + (path.extname(url) ? '' : '.scss'))
          });
        }
      });
    });

    it('should override imports with "file" as input and fires callback with file', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), '');
          done();
        },
        importer: function(url, prev, done) {
          done({
            file: path.resolve(path.dirname(prev), url + (path.extname(url) ? '' : '.scss'))
          });
        }
      });
    });

    it('should override imports with "data" as input and returns file', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), '');
          done();
        },
        importer: function(url, /* jshint unused:false */ prev) {
          return {
            file: path.resolve(path.dirname(fixture('include-files/index.scss')), url + (path.extname(url) ? '' : '.scss'))
          };
        }
      });
    });

    it('should override imports with "file" as input and returns file', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), '');
          done();
        },
        importer: function(url, prev) {
          return {
            file: path.resolve(path.dirname(prev), url + (path.extname(url) ? '' : '.scss'))
          };
        }
      });
    });
	
    it('should override imports with "data" as input and fires callback with contents', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev, done) {
          done({
            contents: 'div {color: yellow;}'
          });
        }
      });
    });

    it('should override imports with "file" as input and fires callback with contents', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function(url, prev, done) {
          done({
            contents: 'div {color: yellow;}'
          });
        }
      });
    });

    it('should override imports with "data" as input and returns contents', function(done) {
      sass.render({
        data: src,
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function() {
          return {
            contents: 'div {color: yellow;}'
          };
        }
      });
    });

    it('should override imports with "file" as input and returns contents', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
          done();
        },
        importer: function() {
          return {
            contents: 'div {color: yellow;}'
          };
        }
      });
    });
  });

  describe('.renderSync(options)', function() {
    it('should compile sass to css with file', function(done) {
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();
      var result = sass.renderSync({file: fixture('simple/index.scss')});

      assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
      done();
    });

    it('should compile sass to css with data', function(done) {
      var src = read(fixture('simple/index.scss'), 'utf8');
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();
      var result = sass.renderSync({data: src});

      assert.equal(result.css.trim(), expected.replace(/\r\n/g, '\n'));
      done();
    });

    it('should compile sass to css using indented syntax', function(done) {
      var src = read(fixture('indent/index.sass'), 'utf8');
      var expected = read(fixture('indent/expected.css'), 'utf8').trim();
      var css = sass.renderSync({
        data: src,
        indentedSyntax: true
      }).css.trim();

      assert.equal(css, expected.replace(/\r\n/g, '\n'));
      done();
    });

    it('should throw error for bad input', function(done) {
      assert.throws(function() {
        sass.renderSync({data: '#navbar width 80%;'});
      });

      done();
    });
  });

  describe('.renderSync(importer)', function() {
    var src = read(fixture('include-files/index.scss'), 'utf8');

    it('should override imports with "data" as input and fires callback with file and contents', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function(url, prev, done) {
          done({
            file: '/some/other/path.scss',
            contents: 'div {color: yellow;}'
          });
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "file" as input and fires callback with file and contents', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function(url, prev, done) {
          done({
            file: '/some/other/path.scss',
            contents: 'div {color: yellow;}'
          });
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "data" as input and returns file and contents', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function(url, prev) {
          return {
            file: prev + url,
            contents: 'div {color: yellow;}'
          };
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "file" as input and returns file and contents', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function(url, prev) {
          return {
            file: prev + url,
            contents: 'div {color: yellow;}'
          };
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "data" as input and fires callback with file', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function(url, /* jshint unused:false */ prev, done) {
          done({
            file: path.resolve(path.dirname(fixture('include-files/index.scss')), url + (path.extname(url) ? '' : '.scss'))
          });
        }
      });

      assert.equal(result.css.trim(), '');
      done();
    });

    it('should override imports with "file" as input and fires callback with file', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function(url, prev, done) {
          done({
            file: path.resolve(path.dirname(prev), url + (path.extname(url) ? '' : '.scss'))
          });
        }
      });

      assert.equal(result.css.trim(), '');
      done();
    });

    it('should override imports with "data" as input and returns file', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function(url, /* jshint unused:false */ prev) {
          return {
            file: path.resolve(path.dirname(fixture('include-files/index.scss')), url + (path.extname(url) ? '' : '.scss'))
          };
        }
      });

      assert.equal(result.css.trim(), '');
      done();
    });

    it('should override imports with "file" as input and returns file', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function(url, prev) {
          return {
            file: path.resolve(path.dirname(prev), url + (path.extname(url) ? '' : '.scss'))
          };
        }
      });

      assert.equal(result.css.trim(), '');
      done();
    });
	
    it('should override imports with "data" as input and fires callback with contents', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function(url, prev, done) {
          done({
            contents: 'div {color: yellow;}'
          });
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "file" as input and fires callback with contents', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function(url, prev, done) {
          done({
            contents: 'div {color: yellow;}'
          });
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "data" as input and returns contents', function(done) {
      var result = sass.renderSync({
        data: src,
        importer: function() {
          return {
            contents: 'div {color: yellow;}'
          };
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });

    it('should override imports with "file" as input and returns contents', function(done) {
      var result = sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function() {
          return {
            contents: 'div {color: yellow;}'
          };
        }
      });

      assert.equal(result.css.trim(), 'div {\n  color: yellow; }\n\ndiv {\n  color: yellow; }');
      done();
    });
  });

  describe('.render({stats: {}})', function() {
    var start = Date.now();

    it('should provide a start timestamp', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert(typeof result.stats.start === 'number');
          assert(result.stats.start >= start);
          done();
        },
        error: function(err) {
          assert(!err);
          done();
        }
      });
    });

    it('should provide an end timestamp', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert(typeof result.stats.end === 'number');
          assert(result.stats.end >= result.stats.start);
          done();
        },
        error: function(err) {
          assert(!err);
          done();
        }
      });
    });

    it('should provide a duration', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert(typeof result.stats.duration === 'number');
          assert.equal(result.stats.end - result.stats.start, result.stats.duration);
          done();
        },
        error: function(err) {
          assert(!err);
          done();
        }
      });
    });

    it('should contain the given entry file', function(done) {
      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.equal(result.stats.entry, fixture('include-files/index.scss'));
          done();
        },
        error: function(err) {
          assert(!err);
          done();
        }
      });
    });

    it('should contain an array of all included files', function(done) {
      var expected = [
        fixture('include-files/bar.scss').replace(/\\/g, '/'),
        fixture('include-files/foo.scss').replace(/\\/g, '/'),
        fixture('include-files/index.scss').replace(/\\/g, '/')
      ];

      sass.render({
        file: fixture('include-files/index.scss'),
        success: function(result) {
          assert.deepEqual(result.stats.includedFiles, expected);
          done();
        },
        error: function(err) {
          assert(!err);
          done();
        }
      });
    });

    it('should contain array with the entry if there are no import statements', function(done) {
      var expected = fixture('simple/index.scss').replace(/\\/g, '/');

      sass.render({
        file: fixture('simple/index.scss'),
        success: function(result) {
          assert.deepEqual(result.stats.includedFiles, [expected]);
          done();
        }
      });
    });

    it('should state `data` as entry file', function(done) {
      sass.render({
        data: read(fixture('simple/index.scss'), 'utf8'),
        success: function(result) {
          assert.equal(result.stats.entry, 'data');
          done();
        }
      });
    });

    it('should contain an empty array as includedFiles', function(done) {
      sass.render({
        data: read(fixture('simple/index.scss'), 'utf8'),
        success: function(result) {
          assert.deepEqual(result.stats.includedFiles, []);
          done();
        }
      });
    });
  });

  describe('.renderSync({stats: {}})', function() {
    var start = Date.now();
    var result = sass.renderSync({
      file: fixture('include-files/index.scss')
    });

    it('should provide a start timestamp', function(done) {
      assert(typeof result.stats.start === 'number');
      assert(result.stats.start >= start);
      done();
    });

    it('should provide an end timestamp', function(done) {
      assert(typeof result.stats.end === 'number');
      assert(result.stats.end >= result.stats.start);
      done();
    });

    it('should provide a duration', function(done) {
      assert(typeof result.stats.duration === 'number');
      assert.equal(result.stats.end - result.stats.start, result.stats.duration);
      done();
    });

    it('should contain the given entry file', function(done) {
      assert.equal(result.stats.entry, resolveFixture('include-files/index.scss'));
      done();
    });

    it('should contain an array of all included files', function(done) {
      var expected = [
        fixture('include-files/bar.scss').replace(/\\/g, '/'),
        fixture('include-files/foo.scss').replace(/\\/g, '/'),
        fixture('include-files/index.scss').replace(/\\/g, '/')
      ];

      assert.equal(result.stats.includedFiles[0], expected[0]);
      assert.equal(result.stats.includedFiles[1], expected[1]);
      assert.equal(result.stats.includedFiles[2], expected[2]);
      done();
    });

    it('should contain array with the entry if there are no import statements', function(done) {
      var expected = fixture('simple/index.scss').replace(/\\/g, '/');

      var result = sass.renderSync({
        file: fixture('simple/index.scss')
      });

      assert.deepEqual(result.stats.includedFiles, [expected]);
      done();
    });

    it('should state `data` as entry file', function(done) {
      var result = sass.renderSync({
        data: read(fixture('simple/index.scss'), 'utf8')
      });

      assert.equal(result.stats.entry, 'data');
      done();
    });

    it('should contain an empty array as includedFiles', function(done) {
      var result = sass.renderSync({
        data: read(fixture('simple/index.scss'), 'utf8')
      });

      assert.deepEqual(result.stats.includedFiles, []);
      done();
    });
  });

  describe('.info()', function() {
    it('should return a correct version info', function(done) {
      assert.equal(sass.info(), [
        'node-sass version: ' + require('../package.json').version, 
        'libsass version: ' + require('../package.json').libsass 
      ].join('\n'));

      done();
    });
  });
});
