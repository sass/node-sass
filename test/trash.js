console.log('trash.js:', __dirname);

var assert = require('assert'),
    fs = require('fs'),
    path = require('path'),
    spawn = require('cross-spawn-async'),
    cli = path.join(__dirname, '..', 'bin', 'node-sass'),
    eol = require('os').EOL;

describe('trash', function() {
  describe('importer', function() {
    it('should not error', function(done) {
      console.log('cli', cli);

      var bin = spawn(cli, [
        'test.scss', '--importer', 'importer.js'
      ]);

      bin.stdout.on('data', function(data) {
        console.log('stdout', eol, data.toString());
      });

      bin.stderr.on('data', function(data) {
        console.log('stderr', eol, data.toString());
        assert.ok(false);
      });

      bin.once('close', function() {
        done();
      });
    });
  });
});
