var path   = require('path'),
    assert = require('assert'),
    fs     = require('fs'),
    exec   = require('child_process').exec,
    spawn  = require('cross-spawn'),
    assign = require('object-assign'),
    cli    = process.env.NODESASS_COVERAGE ? require('../lib-coverage/cli') : require('../lib/cli'),
    cliPath = path.resolve(__dirname, '../bin/node-sass'),
    sampleFilename = path.resolve(__dirname, 'sample.scss');

var expectedSampleCompressed = '#navbar{width:80%;height:23px}\
#navbar ul{list-style-type:none}\
#navbar li{float:left}\
#navbar li a{font-weight:bold}';

var expectedSampleNoComments = '#navbar {\n\
  width: 80%;\n\
  height: 23px; }\n\
\n\
#navbar ul {\n\
  list-style-type: none; }\n\
\n\
#navbar li {\n\
  float: left; }\n\
  #navbar li a {\n\
    font-weight: bold; }\n';

var expectedSampleCustomImagePath = 'body {\n\
  background-image: url("/path/to/images/image.png"); }\n';

var sampleScssPath = path.join(__dirname, 'sample.scss');
var sampleCssOutputPath = path.join(__dirname, '../sample.css');
var sampleCssMapOutputPath = path.join(__dirname, '../sample.css.map');

describe('cli', function() {
  it('should read data from stdin', function(done) {
    this.timeout(6000);
    var src = fs.createReadStream(sampleScssPath);
    var emitter = spawn(cliPath, ['--stdout']);

    emitter.stdout.on('data', function(data) {
      data = data.toString().trim();
      assert.equal(data, expectedSampleNoComments.trim());
      done();
    });

    src.pipe(emitter.stdin);
  });

  it('should write to disk when using --output', function(done) {
    this.timeout(6000);
    var src = fs.createReadStream(sampleScssPath);
    var emitter = spawn(cliPath, ['--output', sampleCssOutputPath], {
      stdio: [null, 'ignore', null]
    });

    emitter.on('close', function() {
      fs.exists(sampleCssOutputPath, function(exists) {
        assert(exists);
        fs.unlink(sampleCssOutputPath, done);
      });
    });

    src.pipe(emitter.stdin);
  });

  it('should treat data as indented code (.sass) if --indented-syntax flag is used', function(done) {
    this.timeout(6000);
    var src = fs.createReadStream(path.join(__dirname, 'indented.sass'));
    var emitter = spawn(cliPath, ['--stdout', '--indented-syntax']);

    // when hit the callback in the following,
    // it means that data is recieved, so we are ok to go.
    emitter.stdout.on('data', function() { done(); }); 
    src.pipe(emitter.stdin);
  });

  it('should print help when run with no arguments', function(done) {
    var env = assign(process.env, { isTTY: true });
    exec('node ' + cliPath, {
      env: env
    }, function(err, stdout, stderr) {
      done(assert(stderr.trim().indexOf('Compile .scss files with node-sass') === 0));
    });
  });

  it('should compile sample.scss as sample.css', function(done) {
    this.timeout(6000);
    var env = assign(process.env, { isTTY: true });
    var resultPath = path.join(__dirname, 'sample.css');

    exec('node ' + cliPath + ' ' + sampleFilename, {
      cwd: __dirname,
      env: env
    }, function(err) {
      assert.equal(err, null);

      fs.exists(resultPath, function(exists) {
        assert(exists);
        fs.unlink(resultPath, done);
      });
    });
  });

  it('should compile sample.scss to ../out.css', function(done) {
    this.timeout(6000);
    var env = assign(process.env, { isTTY: true });
    var resultPath = path.resolve(__dirname, '../out.css');

    exec('node ' + cliPath + ' ' + sampleFilename + ' ../out.css', {
      cwd: __dirname,
      env: env
    }, function(err) {
      assert.equal(err, null);

      fs.exists(resultPath, function(exists) {
        assert(exists);
        fs.unlink(resultPath, done);
      });
    });
  });

  it('should compile with --include-path option', function(done) {
    var emitter = cli([
      '--include-path', path.join(__dirname, 'lib'),
      '--include-path', path.join(__dirname, 'functions'),
      path.join(__dirname, 'include_path.scss')
    ]);
    emitter.on('error', done);
    emitter.on('write', function(err, file, css) {
      assert.equal(css.trim(), 'body {\n  background: red;\n  color: #0000fe; }');
      fs.unlink(file, done);
    });
  });

  it('should compile with the --output-style', function(done) {
    var emitter = cli(['--output-style', 'compressed', sampleScssPath]);
    emitter.on('error', done);
    emitter.on('write', function(err, file, css) {
      assert.equal(css, expectedSampleCompressed);
      fs.unlink(file, done);
    });
  });

  it('should compile with the --source-comments option', function(done) {
    var emitter = cli(['--source-comments', 'none', sampleScssPath]);
    emitter.on('error', done);
    emitter.on('write', function(err, file, css) {
      assert.equal(css, expectedSampleNoComments);
      fs.unlink(file, done);
    });
  });

  it('should compile with the --image-path option', function(done) {
    var emitter = cli(['--image-path', '/path/to/images', path.join(__dirname, 'image_path.scss')]);
    emitter.on('error', done);
    emitter.on('write', function(err, file, css) {
      assert.equal(css, expectedSampleCustomImagePath);
      fs.unlink(file, done);
    });
  });

  it('should write the output to the file specified with the --output option', function(done) {
    var resultPath = path.join(__dirname, '../output.css');
    var emitter = cli(['--output', resultPath, sampleScssPath]);
    emitter.on('error', done);
    emitter.on('write', function() {
      fs.exists(resultPath, function(exists) {
        assert(exists);
        fs.unlink(resultPath, done);
      });
    });
  });

  it('should write to stdout with the --stdout option', function(done) {
    var emitter = cli(['--stdout', sampleScssPath]);
    emitter.on('error', done);
    emitter.on('log', function(css) {
      assert.equal(css, expectedSampleNoComments);
      done();
    });
  });

  it('should not write to disk with the --stdout option', function(done) {
    var emitter = cli(['--stdout', sampleScssPath]);
    emitter.on('error', done);
    emitter.on('done', function() {
      fs.exists(sampleCssOutputPath, function(exists) {
        assert(!exists);
        if (exists) {fs.unlinkSync(sampleCssOutputPath);}
        done();
      });
    });
  });

  it('should not exit with the --watch option', function(done) {
    var command = cliPath + ' --watch ' + sampleScssPath;
    var env = assign(process.env, { isTTY: true });
    var child = exec('node ' + command, {
      env: env
    });
    var exited = false;

    child.on('exit', function() {
      exited = true;
    });

    setTimeout(function() {
      if (exited){
        throw new Error('Watch ended too early!');
      } else {
        child.kill();
        done();
      }
    }, 100);
  });

  it('should compile with the --source-map option', function(done) {
    var emitter = cli([sampleScssPath, '--source-map']);
    emitter.on('error', done);
    emitter.on('write-source-map', function(err, file) {
      assert.equal(file, sampleCssMapOutputPath);
      fs.exists(file, function(exists) {
        assert(exists);
      });
    });

    emitter.on('done', function() {
      fs.unlink(sampleCssMapOutputPath, function() {
        fs.unlink(sampleCssOutputPath, function() {
          done();
        });
      });
    });
  });

  it('should compile with the --source-map option with specific filename', function(done){
    var emitter = cli([sampleScssPath, '--source-map', path.join(__dirname, '../sample.map')]);
    emitter.on('error', done);
    emitter.on('write-source-map', function(err, file) {
      assert.equal(file, path.join(__dirname, '../sample.map'));
      fs.exists(file, function(exists) {
        assert(exists);
      });
    });
    emitter.on('done', function() {
      fs.unlink(path.join(__dirname, '../sample.map'), function() {
        fs.unlink(sampleCssOutputPath, function() {
          done();
        });
      });
    });
  });

  it('should compile a sourceMap if --source-comments="map", but the --source-map option is excluded', function(done) {
    var emitter = cli([sampleScssPath, '--source-comments', 'map']);
    emitter.on('error', done);
    emitter.on('write-source-map', function(err, file) {
      assert.equal(file, sampleCssMapOutputPath);
      fs.exists(file, function(exists) {
        assert(exists);
      });
    });
    emitter.on('done', function() {
      fs.unlink(sampleCssMapOutputPath, function() {
        fs.unlink(sampleCssOutputPath, function() {
          done();
        });
      });
    });
  });

  it('should omit a sourceMappingURL from CSS if --omit-source-map-url flag is used', function(done) {
    var emitter = cli([sampleScssPath, '--source-map', path.join(__dirname, '../sample.map'), '--omit-source-map-url']);
    emitter.on('error', done);
    emitter.on('done', function() {
      fs.exists(sampleCssOutputPath, function(exists) {
        assert.ok(fs.readFileSync(sampleCssOutputPath, 'utf8').indexOf('sourceMappingURL=') === -1);
        if (exists) {fs.unlinkSync(sampleCssOutputPath);}
        done();
      });
    });
  });
});
