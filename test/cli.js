var assert = require('assert'),
    fs = require('fs'),
    path = require('path'),
    read = require('fs').readFileSync,
    spawn = require('cross-spawn'),
    cli = path.join(__dirname, '..', 'bin', 'node-sass'),
    fixture = path.join.bind(null, __dirname, 'fixtures');

describe('cli', function() {
  describe('node-sass < in.scss', function() {
    it('should read data from stdin', function(done) {
      var src = fs.createReadStream(fixture('simple/index.scss'));
      var expected = read(fixture('simple/expected.css'), 'utf8').trim();
      var bin = spawn(cli, ['--stdout']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile sass using the --indented-syntax option', function(done) {
      var src = fs.createReadStream(fixture('indent/index.sass'));
      var expected = read(fixture('indent/expected.css'), 'utf8').trim();
      var bin = spawn(cli, ['--stdout', '--indented-syntax']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --output-style option', function(done) {
      var src = fs.createReadStream(fixture('compressed/index.scss'));
      var expected = read(fixture('compressed/expected.css'), 'utf8').trim();
      var bin = spawn(cli, ['--stdout', '--output-style', 'compressed']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --source-comments option', function(done) {
      var src = fs.createReadStream(fixture('source-comments/index.scss'));
      var expected = read(fixture('source-comments/expected.css'), 'utf8').trim();
      var bin = spawn(cli, ['--stdout', '--source-comments']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --image-path option', function(done) {
      var src = fs.createReadStream(fixture('image-path/index.scss'));
      var expected = read(fixture('image-path/expected.css'), 'utf8').trim();
      var bin = spawn(cli, ['--stdout', '--image-path', '/path/to/images']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });

      src.pipe(bin.stdin);
    });
  });

  describe('node-sass in.scss', function() {
    it('should compile a scss file', function(done) {
      process.chdir(fixture('simple'));

      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index.css');
      var bin = spawn(cli, [src]);

      bin.on('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        process.chdir(__dirname);
        done();
      });
    });

    it('should compile with the --include-path option', function(done) {
      var includePaths = [
        '--include-path', fixture('include-path/functions'),
        '--include-path', fixture('include-path/lib')
      ];

      var src = fixture('include-path/index.scss');
      var expected = read(fixture('include-path/expected.css'), 'utf8').trim();
      var bin = spawn(cli, [src, '--stdout'].concat(includePaths));

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), expected.replace(/\r\n/g, '\n'));
        done();
      });
    });

    it('should not exit with the --watch option', function(done) {
      var src = fixture('simple/index.scss');
      var bin = spawn(cli, [src, '--stdout', '--watch']);
      var exited;

      bin.on('close', function () {
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

    it('should emit `warn` on file change when using --watch option', function(done) {
      fs.writeFileSync(fixture('simple/tmp.scss'), '');

      var src = fixture('simple/tmp.scss');
      var bin = spawn(cli, [src, '--stdout', '--watch']);

      bin.stderr.setEncoding('utf8');
      bin.stderr.on('data', function(data) {
        assert(data.trim() === '=> changed: ' + src);
        bin.kill();
        fs.unlinkSync(src);
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(src, 'body {}');
      }, 500);
    });

    it('should render all watched files', function(done) {
      fs.writeFileSync(fixture('simple/foo.scss'), '');
      fs.writeFileSync(fixture('simple/bar.scss'), '');

      var src = fixture('simple/foo.scss');
      var watched = fixture('simple/bar.scss');
      var bin = spawn(cli, [
        src, '--stdout', '--watch', watched,
        '--output-style', 'compressed'
      ]);

      bin.stdout.setEncoding('utf8');
      bin.stdout.on('data', function(data) {
        assert(data.trim() === 'body{background:white}');
        bin.kill();
        fs.unlinkSync(src);
        fs.unlinkSync(watched);
        done();
      });

      setTimeout(function() {
        fs.appendFileSync(watched, 'body{background:white}');
      }, 500);
    });
  });

  describe('node-sass in.scss --output out.css', function() {
    it('should compile a scss file to build.css', function(done) {
      var src = fixture('simple/index.scss');
      var dest = fixture('simple/index.css');
      var bin = spawn(cli, [src, '--output', path.dirname(dest)]);

      bin.on('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        done();
      });
    });

    it('should compile with the --source-map option', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/index.css');
      var expected = read(fixture('source-map/expected.css'), 'utf8').trim().replace(/\r\n/g, '\n');
      var map = fixture('source-map/index.map');
      var bin = spawn(cli, [src, '--output', path.dirname(dest), '--source-map', map]);

      bin.on('close', function () {
        assert.equal(read(dest, 'utf8').trim(), expected);
        assert(fs.existsSync(map));
        fs.unlinkSync(map);
        fs.unlinkSync(dest);
        done();
      });
    });

    it('should omit sourceMappingURL if --omit-source-map-url flag is used', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/index.css');
      var map = fixture('source-map/index.map');
      var bin = spawn(cli, [
        src, '--output', path.dirname(dest),
        '--source-map', map, '--omit-source-map-url'
      ]);

      bin.on('close', function () {
        assert(read(dest, 'utf8').indexOf('sourceMappingURL') === -1);
        assert(fs.existsSync(map));
        fs.unlinkSync(map);
        fs.unlinkSync(dest);
        done();
      });
    });
  });
});
