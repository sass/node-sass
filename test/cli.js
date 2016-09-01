var assert = require('assert'),
  fs = require('fs'),
  path = require('path'),
  read = require('fs').readFileSync,
  glob = require('glob'),
  rimraf = require('rimraf'),
  stream = require('stream'),
  spawn = require('cross-spawn'),
  cli = path.join(__dirname, '..', 'bin', 'node-sass'),
  fixture = path.join.bind(null, __dirname, 'fixtures'),
  LIBSASS_VERSION = null;

var Time = require('time-diff');
var time = new Time();

describe('cli', function() {
  before(function(done) {
    var fin = false;
    var diff = time.diff('before');

    diff('before spawn');
    var bin = spawn(cli, ['-v']);
    bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });
    bin.stdout.on('data', function(data) {
      diff('stdout.data');
      console.log('stdout.data', data);

      LIBSASS_VERSION = data.trim().split(['\n'])
          .filter(function(a) { return a.substr(0,7) === 'libsass'; })[0]
          .split('\t')[1];
      if (!fin) {
        fin = true
        done();
      }
    });

    bin.stderr.on('data', function(data) {
      diff('stderr.data');
      console.log('stderr.data', data.toString());
      if (!fin) {
        fin = true
        done();
      }
    });

    bin.once('close', function() {
      diff('bin.close');
      if (!fin) {
        fin = true
        done();
      }
    });
  });

  describe('node-sass < in.scss', function() {
    it('should read data from stdin', function(done) {
      var diff = time.diff('should read data from stdin');
      var src = fs.createReadStream(fixture('simple/index.scss'));
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile sass using the --indented-syntax option', function(done) {
      var diff = time.diff('should compile sass using the --indented-syntax option');
      var src = fs.createReadStream(fixture('indent/index.sass'));
      var expected = read(fixture('indent/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli, ['--indented-syntax']);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --quiet option', function(done) {
      var diff = time.diff('should compile with the --quiet option');
      var src = fs.createReadStream(fixture('simple/index.scss'));
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli, ['--quiet']);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --output-style option', function(done) {
      var diff = time.diff('should compile with the --output-style option');
      var src = fs.createReadStream(fixture('compressed/index.scss'));
      var expected = read(fixture('compressed/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli, ['--output-style', 'compressed']);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --source-comments option', function(done) {
      var diff = time.diff('should compile with the --source-comments option');
      var src = fs.createReadStream(fixture('source-comments/index.scss'));
      var expected = read(fixture('source-comments/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli, ['--source-comments']);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should render with indentWidth and indentType options', function(done) {
      var diff = time.diff('should render with indentWidth and indentType options');
      var src = new stream.Readable();
      diff('before spawn');
      var bin = spawn(cli, ['--indent-width', 7, '--indent-type', 'tab']);

      src._read = function() { };
      src.push('div { color: transparent; }');
      src.push(null);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), 'div {\n\t\t\t\t\t\t\tcolor: transparent; }');
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should render with linefeed option', function(done) {
      var diff = time.diff('should render with linefeed option');
      var src = new stream.Readable();
      diff('before spawn');
      var bin = spawn(cli, ['--linefeed', 'lfcr']);

      src._read = function() { };
      src.push('div { color: transparent; }');
      src.push(null);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        diff('stdout.data');
        assert.equal(data.trim(), 'div {\n\r  color: transparent; }');
        done();
      });

      src.pipe(bin.stdin);
    });
  });

  describe('node-sass in.scss', function() {
    it('should compile a scss file', function(done) {
      var diff = time.diff('should compile a scss file');
      process.chdir(fixture('simple'));

      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index.css');
      diff('before spawn');
      var bin = spawn(cli, [src, dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.once('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        process.chdir(__dirname);
        done();
      });
    });

    it('should compile a scss file to custom destination', function(done) {
      var diff = time.diff('should compile a scss file to custom destination');
      process.chdir(fixture('simple'));

      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index-custom.css');
      diff('before spawn');
      var bin = spawn(cli, [src, dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.once('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        process.chdir(__dirname);
        done();
      });
    });

    it('should compile with the --include-path option', function(done) {
      var diff = time.diff('should compile with the --include-path option');
      var includePaths = [
        '--include-path', fixture('include-path/functions'),
        '--include-path', fixture('include-path/lib')
      ];

      var src = fixture('include-path/index.scss');
      var expected = read(fixture('include-path/expected.css'), 'utf8').trim();
      diff('before spawn');
      var bin = spawn(cli, [src].concat(includePaths));

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });
    });

    it('should compile silently using the --quiet option', function(done) {
      var diff = time.diff('should compile silently using the --quiet option');
      process.chdir(fixture('simple'));

      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index.css');
      diff('before spawn');
      var bin = spawn(cli, [src, dest, '--quiet']);
      var didEmit = false;

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.once('data', function() {
        didEmit = true;
      });

      bin.once('close', function() {
        assert.equal(didEmit, false);
        fs.unlinkSync(dest);
        process.chdir(__dirname);
        done();
      });
    });

    it('should still report errors with the --quiet option', function(done) {
      var diff = time.diff('should still report errors with the --quiet option');
      process.chdir(fixture('invalid'));

      var src = fixture('invalid/index.scss');
      var dest = fixture('invalid/index.css');
      diff('before spawn');
      var bin = spawn(cli, [src, dest, '--quiet']);
      var didEmit = false;

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.once('data', function() {
        didEmit = true;
      });

      bin.once('close', function() {
        assert.equal(didEmit, true);
        process.chdir(__dirname);
        done();
      });
    });

    it('should not exit with the --watch option', function(done) {
      var diff = time.diff('should not exit with the --watch option');
      var src = fixture('simple/index.scss');
      diff('before spawn');
      var bin = spawn(cli, [src, '--watch']);
      var exited;

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.once('close', function() {
        exited = true;
      });

      setTimeout(function() {
        if (exited) {
          throw new Error('Watch ended too early!');
        } else {
          bin.kill();
          done();
        }
      }, 100);
    });

    it.skip('should emit `warn` on file change when using --watch option', function(done) {
      var src = fixture('simple/tmp.scss');

      fs.writeFileSync(src, '');

      var bin = spawn(cli, ['--watch', src]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.setEncoding('utf8');
      bin.stderr.once('data', function(data) {
        assert.strictEqual(data.trim(), '=> changed: ' + src);
        fs.unlinkSync(src);
        bin.kill();
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(src, 'body {}');
      }, 500);
    });

    it.skip('should emit nothing on file change when using --watch and --quiet options', function(done) {
      var src = fixture('simple/tmp.scss');
      var didEmit = false;
      fs.writeFileSync(src, '');

      var bin = spawn(cli, ['--watch', '--quiet', src]);

      bin.stderr.setEncoding('utf8');

      bin.stderr.once('data', function() {
        didEmit = true;
      });

      setTimeout(function() {
        fs.appendFileSync(src, 'body {}');
        setTimeout(function() {
          assert.equal(didEmit, false);
          bin.kill();
          done();
          fs.unlinkSync(src);
        }, 200);
      }, 500);
    });

    it.skip('should render all watched files', function(done) {
      var src = fixture('simple/bar.scss');

      fs.writeFileSync(src, '');

      var bin = spawn(cli, [
        '--output-style', 'compressed',
        '--watch', src
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        assert.strictEqual(data.trim(), 'body{background:white}');
        fs.unlinkSync(src);
        bin.kill();
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(src, 'body{background:white}');
      }, 500);
    });

    it.skip('should watch the full scss dep tree for a single file (scss)', function(done) {
      var src = fixture('watching/index.scss');
      var foo = fixture('watching/white.scss');

      fs.writeFileSync(foo, '');

      var bin = spawn(cli, [
        '--output-style', 'compressed',
        '--watch', src
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        assert.strictEqual(data.trim(), 'body{background:blue}');
        bin.kill();
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(foo, 'body{background:blue}\n');
      }, 500);
    });

    it.skip('should watch the full sass dep tree for a single file (sass)', function(done) {
      var src = fixture('watching/index.sass');
      var foo = fixture('watching/bar.sass');

      fs.writeFileSync(foo, '');

      var bin = spawn(cli, [
        '--output-style', 'compressed',
        '--watch', src
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.stdout.once('data', function(data) {
        assert.strictEqual(data.trim(), 'body{background:red}');
        bin.kill();
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(foo, 'body\n\tbackground: red\n');
      }, 500);
    });
  });

  // describe('node-sass --output directory', function() {
  //   it.skip('should watch whole directory', function(done) {
  //     var destDir = fixture('watching-css-out-01/');
  //     var srcDir = fixture('watching-dir-01/');
  //     var srcFile = path.join(srcDir, 'index.scss');

  //     fs.writeFileSync(srcFile, '');

  //     var bin = spawn(cli, [
  //       '--output-style', 'compressed',
  //       '--output', destDir,
  //       '--watch', srcDir
  //     ]);

  //     setTimeout(function() {
  //       fs.appendFileSync(srcFile, 'a {color:green;}\n');
  //       setTimeout(function() {
  //         bin.kill();
  //         var files = fs.readdirSync(destDir);
  //         assert.deepEqual(files, ['index.css']);
  //         rimraf(destDir, done);
  //       }, 200);
  //     }, 500);
  //   });

  //   it.skip('should compile all changed files in watched directory', function(done) {
  //     var destDir = fixture('watching-css-out-02/');
  //     var srcDir = fixture('watching-dir-02/');
  //     var srcFile = path.join(srcDir, 'foo.scss');

  //     fs.writeFileSync(srcFile, '');

  //     var bin = spawn(cli, [
  //       '--output-style', 'compressed',
  //       '--output', destDir,
  //       '--watch', srcDir
  //     ]);

  //     setTimeout(function () {
  //       fs.appendFileSync(srcFile, 'body{background:white}\n');
  //       setTimeout(function () {
  //         bin.kill();
  //         var files = fs.readdirSync(destDir);
  //         assert.deepEqual(files, ['foo.css', 'index.css']);
  //         rimraf(destDir, done);
  //       }, 200);
  //     }, 500);
  //   });
  // });

  describe('node-sass in.scss --output out.css', function() {
    var diff;

    beforeEach(function() {
      diff = time.diff('spec');
    });

    it('should compile a scss file to build.css', function(done) {
      var diff = time.diff('should compile a scss file to build.css');
      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index.css');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', path.dirname(dest)]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should compile with the --source-map option', function(done) {
      var diff = time.diff('should compile with the --source-map option');
      if (LIBSASS_VERSION < '3.3') {
        this.skip('Source map functionality broken in libsass < 3.3');
      }

      var src = fixture('source-map/index.scss');
      var destCss = fixture('source-map/index.css');
      var destMap = fixture('source-map/index.map');
      var expectedCss = read(fixture('source-map/expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      var expectedMap = read(fixture('source-map/expected.map'), 'utf8').trim().replace(/\r\n/g, '\n');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', path.dirname(destCss), '--source-map', destMap]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(destCss, 'utf8').trim(), expectedCss);
        assert.equal(read(destMap, 'utf8').trim(), expectedMap);
        fs.unlinkSync(destCss);
        fs.unlinkSync(destMap);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should omit sourceMappingURL if --omit-source-map-url flag is used', function(done) {
      var diff = time.diff('should omit sourceMappingURL if --omit-source-map-url flag is used');
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/index.css');
      var map = fixture('source-map/index.map');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--source-map', map, '--omit-source-map-url'
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.strictEqual(read(dest, 'utf8').indexOf('sourceMappingURL'), -1);
        assert(fs.existsSync(map));
        fs.unlinkSync(map);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should compile with the --source-root option', function(done) {
      var diff = time.diff('should compile with the --source-root option');
      var src = fixture('source-map/index.scss');
      var destCss = fixture('source-map/index.css');
      var destMap = fixture('source-map/index.map');
      var expectedCss = read(fixture('source-map/expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      var expectedUrl = 'http://test/';
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(destCss),
        '--source-map-root', expectedUrl,
        '--source-map', destMap
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(destCss, 'utf8').trim(), expectedCss);
        assert.equal(JSON.parse(read(destMap, 'utf8')).sourceRoot, expectedUrl);
        fs.unlinkSync(destCss);
        fs.unlinkSync(destMap);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should compile with the --source-map-embed option and no outfile', function(done) {
      var diff = time.diff('should compile with the --source-map-embed option and no outfile');
      var src = fixture('source-map-embed/index.scss');
      var expectedCss = read(fixture('source-map-embed/expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      var result = '';
      diff('before spawn');
      var bin = spawn(cli, [
        src,
        '--source-map-embed',
        '--source-map', 'true'
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stdout.on('data', function(data) {
        diff('stdout.data');
        result += data;
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(result.trim().replace(/\r\n/g, '\n'), expectedCss);
        diff('bin.close.cleanup');
        done();
      });
    });
  });

  describe('node-sass sass/ --output css/', function() {
    it('should create the output directory', function(done) {
      var diff = time.diff('should create the output directory');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/css');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert(fs.existsSync(dest));
        rimraf.sync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should compile all files in the folder', function(done) {
      var diff = time.diff('should compile all files in the folder');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/css');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var files = fs.readdirSync(dest).sort();
        assert.deepEqual(files, ['one.css', 'two.css', 'nested'].sort());
        var nestedFiles = fs.readdirSync(path.join(dest, 'nested'));
        assert.deepEqual(nestedFiles, ['three.css']);
        rimraf.sync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should compile with --source-map set to directory', function(done) {
      var diff = time.diff('should compile with --source-map set to directory');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/css');
      var destMap = fixture('input-directory/map');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', dest, '--source-map', destMap]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var map = JSON.parse(read(fixture('input-directory/map/nested/three.css.map'), 'utf8'));

        assert.equal(map.file, '../../css/nested/three.css');
        rimraf.sync(dest);
        rimraf.sync(destMap);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should skip files with an underscore', function(done) {
      var diff = time.diff('should skip files with an underscore');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/css');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var files = fs.readdirSync(dest);
        assert.equal(files.indexOf('_skipped.css'), -1);
        rimraf.sync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should ignore nested files if --recursive false', function(done) {
      var diff = time.diff('should ignore nested files if --recursive false');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/css');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', dest,
        '--recursive', false
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var files = fs.readdirSync(dest);
        assert.deepEqual(files, ['one.css', 'two.css']);
        rimraf.sync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should error if no output directory is provided', function(done) {
      var diff = time.diff('should error if no output directory is provided');
      var src = fixture('input-directory/sass');
      diff('before spawn');
      var bin = spawn(cli, [src]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function(code) {
        diff('bin.close');
        assert.notStrictEqual(code, 0);
        assert.strictEqual(glob.sync(fixture('input-directory/**/*.css')).length, 0);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should error if output directory is not a directory', function(done) {
      var diff = time.diff('should error if output directory is not a directory');
      var src = fixture('input-directory/sass');
      var dest = fixture('input-directory/sass/one.scss');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function(code) {
        diff('bin.close');
        assert.notStrictEqual(code, 0);
        assert.equal(glob.sync(fixture('input-directory/**/*.css')).length, 0);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should not error if output directory is a symlink', function(done) {
      var diff = time.diff('should not error if output directory is a symlink');
      var outputDir = fixture('input-directory/css');
      var src = fixture('input-directory/sass');
      var symlink = fixture('symlinked-css');
      fs.mkdirSync(outputDir);
      fs.symlinkSync(outputDir, symlink);
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', symlink]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var files = fs.readdirSync(outputDir).sort();
        assert.deepEqual(files, ['one.css', 'two.css', 'nested'].sort());
        var nestedFiles = fs.readdirSync(path.join(outputDir, 'nested'));
        assert.deepEqual(nestedFiles, ['three.css']);
        rimraf.sync(outputDir);
        fs.unlinkSync(symlink);
        diff('bin.close.cleanup');
        done();
      });
    });
  });

  describe('node-sass in.scss --output path/to/file/out.css', function() {
    it('should create the output directory', function(done) {
      var diff = time.diff('should create the output directory');
      var src = fixture('output-directory/index.scss');
      var dest = fixture('output-directory/path/to/file/index.css');
      diff('before spawn');
      var bin = spawn(cli, [src, '--output', path.dirname(dest)]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert(fs.existsSync(path.dirname(dest)));
        fs.unlinkSync(dest);
        fs.rmdirSync(path.dirname(dest));
        dest = path.dirname(dest);
        fs.rmdirSync(path.dirname(dest));
        dest = path.dirname(dest);
        fs.rmdirSync(path.dirname(dest));
        diff('bin.close.cleanup');
        done();
      });
    });

  });

  describe('node-sass --follow --output output-dir input-dir', function() {
    it('should compile with the --follow option', function(done) {
      var diff = time.diff('should compile with the --follow option');
      var src = fixture('follow/input-dir');
      var dest = fixture('follow/output-dir');

      fs.mkdirSync(src);
      fs.symlinkSync(path.join(path.dirname(src), 'foo'), path.join(src, 'foo'), 'dir');

      diff('before spawn');
      var bin = spawn(cli, [src, '--follow', '--output', dest]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        var expected = path.join(dest, 'foo/bar/index.css');
        fs.unlinkSync(path.join(src, 'foo'));
        fs.rmdirSync(src);
        assert(fs.existsSync(expected));
        fs.unlinkSync(expected);
        expected = path.dirname(expected);
        fs.rmdirSync(expected);
        expected = path.dirname(expected);
        fs.rmdirSync(expected);
        fs.rmdirSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });
  });

  describe('importer', function() {
    var dest = fixture('include-files/index.css');
    var src = fixture('include-files/index.scss');
    var expected = read(fixture('include-files/expected-importer.css'), 'utf8').trim().replace(/\r\n/g, '\n');

    it('should override imports and fire callback with file and contents', function(done) {
      var diff = time.diff('should override imports and fire callback with file and contents');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_file_and_data_cb.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should override imports and fire callback with file', function(done) {
      var diff = time.diff('should override imports and fire callback with file');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_file_cb.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        if (fs.existsSync(dest)) {
          assert.equal(read(dest, 'utf8').trim(), '');
          fs.unlinkSync(dest);
        }

        diff('bin.close.cleanup');
        done();
      });
    });

    it('should override imports and fire callback with data', function(done) {
      var diff = time.diff('should override imports and fire callback with data');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_data_cb.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should override imports and return file and contents', function(done) {
      var diff = time.diff('should override imports and return file and contents');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_file_and_data.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should override imports and return file', function(done) {
      var diff = time.diff('should override imports and return file');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_file.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        if (fs.existsSync(dest)) {
          assert.equal(read(dest, 'utf8').trim(), '');
          fs.unlinkSync(dest);
        }

        diff('bin.close.cleanup');
        done();
      });
    });

    it('should override imports and return data', function(done) {
      var diff = time.diff('should override imports and return data');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_data.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should accept arrays of importers and return respect the order', function(done) {
      var diff = time.diff('should accept arrays of importers and return respect the order');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_arrays_of_importers.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should return error for invalid importer file path', function(done) {
      var diff = time.diff('should return error for invalid importer file path');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('non/existing/path')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function(code) {
        diff('bin.close');
        assert.notStrictEqual(code, 0);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should reflect user-defined Error', function(done) {
      var diff = time.diff('should reflect user-defined Error');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--importer', fixture('extras/my_custom_importer_error.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.once('data', function(code) {
        diff('stderr.data');
        assert.equal(JSON.parse(code).message, 'doesn\'t exist!');
        diff('bin.close.cleanup');
        done();
      });
    });
  });

  describe('functions', function() {
    it('should let custom functions call setter methods on wrapped sass values (number)', function(done) {
      var diff = time.diff('should let custom functions call setter methods on wrapped sass values (number)');
      var dest = fixture('custom-functions/setter.css');
      var src = fixture('custom-functions/setter.scss');
      var expected = read(fixture('custom-functions/setter-expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--functions', fixture('extras/my_custom_functions_setter.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });

    it('should properly convert strings when calling custom functions', function(done) {
      var diff = time.diff('should properly convert strings when calling custom functions');
      var dest = fixture('custom-functions/string-conversion.css');
      var src = fixture('custom-functions/string-conversion.scss');
      var expected = read(fixture('custom-functions/string-conversion-expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      diff('before spawn');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--functions', fixture('extras/my_custom_functions_string_conversion.js')
      ]);

      bin.stdout.setEncoding('utf8');
      bin.on('disconnect', function(err) {
        console.log('disconnect', err);
        diff('disconnect');
      });
      bin.on('exit', function(code, signal) {
        console.log('exit', code, signal);
        diff('exit');
      });
      bin.on('error', function(data) {
        console.log('error', data.toString());
        diff('error');
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr.data', data.toString());
        diff('stderr.data');
      });
      bin.once('close', function() {
        diff('bin.close');
        assert.equal(read(dest, 'utf8').trim(), expected);
        fs.unlinkSync(dest);
        diff('bin.close.cleanup');
        done();
      });
    });
  });
});
