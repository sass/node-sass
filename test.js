var assert = require('assert'),
    eol = require('os').EOL
    path = require('path'),
    readFileSync = require('fs').readFileSync,
    sassPath = require.resolve('./lib'),
    sass = require(sassPath),
    spawn = require('cross-spawn-async'),
    cli = path.join(__dirname, 'bin', 'node-sass');

describe('test', function() {
  it('should not error', function(done) {
    var bin = spawn(cli, [
      '--importer', 'importer.js', 'test.scss'
    ]);

    bin.stdout.on('data', function(data) {
      console.log('stdout', eol, data.toString());
    });

    bin.stderr.on('data', function(data) {
      console.log('stderr', eol, data.toString());
      assert.ifError(true);
    });

    bin.once('close', function() {
      console.log('close');
      done();
    });
  });
});