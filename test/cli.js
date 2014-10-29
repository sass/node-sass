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
      var expected = fixture('simple/expected.css');
      var bin = spawn(cli, ['--stdout']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should write to disk when using --output', function(done) {
      var src = fs.createReadStream(fixture('simple/index.scss'));
      var dest = fixture('simple/build.css');
      var bin = spawn(cli, ['--output', dest]);

      bin.on('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile sass using the --indented-syntax option', function(done) {
      var src = fs.createReadStream(fixture('indent/index.sass'));
      var expected = fixture('indent/expected.css');
      var bin = spawn(cli, ['--stdout', '--indented-syntax']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --output-style option', function(done) {
      var src = fs.createReadStream(fixture('compressed/index.scss'));
      var expected = fixture('compressed/expected.css');
      var bin = spawn(cli, ['--stdout', '--output-style', 'compressed']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --source-comments option', function(done) {
      var src = fs.createReadStream(fixture('source-comments/index.scss'));
      var expected = fixture('source-comments/expected.css');
      var bin = spawn(cli, ['--stdout', '--source-comments']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
        done();
      });

      src.pipe(bin.stdin);
    });

    it('should compile with the --image-path option', function(done) {
      var src = fs.createReadStream(fixture('image-path/index.scss'));
      var expected = fixture('image-path/expected.css');
      var bin = spawn(cli, ['--stdout', '--image-path', '/path/to/images']);

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
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
      var expected = fixture('include-path/expected.css');
      var bin = spawn(cli, [src, '--stdout'].concat(includePaths));

      bin.stdout.setEncoding('utf8');
      bin.stdout.once('data', function(data) {
        assert.equal(data.trim(), read(expected, 'utf8').trim());
        done();
      });
    });

    it('should not exit with the --watch option', function(done) {
      var src = fixture('simple/index.scss');
      var bin = spawn(cli, ['--stdout', '--watch', src]);
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
  });

  describe('node-sass in.scss --output out.css', function() {
    it('should compile a scss file to build.css', function(done) {
      var src = fixture('simple/index.scss');
      var dest = fixture('simple/build.css');
      var bin = spawn(cli, [src, '--output', dest]);

      bin.on('close', function() {
        assert(fs.existsSync(dest));
        fs.unlinkSync(dest);
        done();
      });
    });

    it('should compile with the --source-map option', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/build.css');
      var expected = fixture('source-map/expected.css');
      var map = fixture('source-map/index.map');
      var bin = spawn(cli, [src, '--output', dest, '--source-map', map]);

      bin.on('close', function () {
        assert.equal(read(dest, 'utf8').trim(), read(expected, 'utf8').trim());
        assert(fs.existsSync(map));
        fs.unlinkSync(map);
        fs.unlinkSync(dest);
        done();
      });
    });

    it('should omit sourceMappingURL if --omit-source-map-url flag is used', function(done) {
      var src = fixture('source-map/index.scss');
      var dest = fixture('source-map/build.css');
      var map = fixture('source-map/index.map');
      var bin = spawn(cli, [src, '--output', dest, '--source-map', map, '--omit-source-map-url']);

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
