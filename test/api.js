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

    it('should compile sass to css with outFile set to absolute url', function(done) {
      sass.render({
        file: fixture('simple/index.scss'),
        sourceMap: true,
        outFile: fixture('simple/index-test.css'),

        success: function(result) {
          assert.equal(JSON.parse(result.map).file, 'index-test.css');
          done();
        }
      });
    });

    it('should compile sass to css with outFile set to relative url', function(done) {
      sass.render({
        file: fixture('simple/index.scss'),
        sourceMap: true,
        outFile: './index-test.css',

        success: function(result) {
          assert.equal(JSON.parse(result.map).file, 'index-test.css');
          done();
        }
      });
    });

    it('should compile sass to css with outFile and sourceMap set to relative url', function(done) {
      sass.render({
        file: fixture('simple/index.scss'),
        sourceMap: './deep/nested/index.map',
        outFile: './index-test.css',

        success: function(result) {
          assert.equal(JSON.parse(result.map).file, '../../index-test.css');
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

    it('should throw error when libsass binary is missing.', function(done) {
      var originalBin = path.join('vendor', process.sassBinaryName, 'binding.node'),
          renamedBin = [originalBin, '_moved'].join('');

      assert.throws(function() {
        // un-require node-sass
        var resolved = require.resolve('../lib');
        delete require.cache[resolved];

        fs.renameSync(originalBin, renamedBin);
        // try to re-require it
        require('../lib');
      }, function(err) {
        if ((err instanceof Error) && /`libsass` bindings not found. Try reinstalling `node-sass`?/.test(err)) {
          fs.renameSync(renamedBin, originalBin);
          done();
          return true;
        }
      });
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

    it('should be able to see its options in this.options', function(done) {
      var fxt = fixture('include-files/index.scss');
      sass.render({
        file: fxt,
        success: function() {
          assert.equal(fxt, this.options.file);
          done();
        },
        importer: function() {
          assert.equal(fxt, this.options.file);
          return {};
        }
      });
    });

    it('should be able to access a persistent options object', function(done) {
      sass.render({
        data: src,
        success: function() {
          assert.equal(this.state, 2);
          done();
        },
        importer: function() {
          this.state = this.state || 0;
          this.state++;
          return {
            contents: 'div {color: yellow;}'
          };
        }
      });
    });

    it('should copy all options properties', function(done) {
      var options;
      options = {
        data: src,
        success: function() {
          assert.strictEqual(this.options.success, options.success);
          done();
        },
        importer: function() {
          assert.strictEqual(this.options.importer, options.importer);
          return {
            contents: 'div {color: yellow;}'
          };
        }
      };
      sass.render(options);
    });
  });

  describe('.render(options, cb)', function() {
    it('should compile sass to css with file', function(done) {
      var expected = 'div {\n  color: yellow; }';
      sass.render({
        data: 'div {color: yellow;}'
      }, function(err, result) {
        assert.equal(result.css.trim(), expected);
        done();
      });
    });

    it('should throw error status 1 for bad input', function(done) {
      sass.render({
        data: '#navbar width 80%;'
      }, function(err) {
        assert(err.message);
        assert.equal(err.status, 1);
        done();
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

    it('should compile sass to css with outFile set to absolute url', function(done) {
      var result = sass.renderSync({
        file: fixture('simple/index.scss'),
        sourceMap: true,
        outFile: fixture('simple/index-test.css')
      });

      assert.equal(JSON.parse(result.map).file, 'index-test.css');
      done();
    });

    it('should compile sass to css with outFile set to relative url', function(done) {
      var result = sass.renderSync({
        file: fixture('simple/index.scss'),
        sourceMap: true,
        outFile: './index-test.css'
      });

      assert.equal(JSON.parse(result.map).file, 'index-test.css');
      done();
    });

    it('should compile sass to css with outFile and sourceMap set to relative url', function(done) {
      var result = sass.renderSync({
        file: fixture('simple/index.scss'),
        sourceMap: './deep/nested/index.map',
        outFile: './index-test.css'
      });

      assert.equal(JSON.parse(result.map).file, '../../index-test.css');
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

    it('should be able to see its options in this.options', function(done) {
      var fxt = fixture('include-files/index.scss');
      var sync = false;
      sass.renderSync({
        file: fixture('include-files/index.scss'),
        importer: function() {
          assert.equal(fxt, this.options.file);
          sync = true;
          return {};
        }
      });
      assert.equal(sync, true);
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
