var assert = require('assert'),
  extensionsPath = process.env.NODESASS_COV
      ? require.resolve('../lib-cov/extensions')
      : require.resolve('../lib/extensions');

describe('runtime parameters', function() {
  var packagePath = require.resolve('../package'),
        // Let's use JSON to fake a deep copy
    savedArgv = JSON.stringify(process.argv),
    savedEnv = JSON.stringify(process.env);

  beforeEach(function() {
    require(packagePath);
    delete require.cache[extensionsPath];
  });

  afterEach(function() {
    delete require.cache[packagePath];
    delete require.cache[extensionsPath];
    process.argv = JSON.parse(savedArgv);
    process.env = JSON.parse(savedEnv);
    require(packagePath);
  });

  describe('configuration precedence should be respected', function() {

    describe('SASS_BINARY_NAME', function() {
      beforeEach(function() {
        process.argv.push('--sass-binary-name', 'aaa');
        process.env.SASS_BINARY_NAME = 'bbb';
        process.env.npm_config_sass_binary_name = 'ccc';
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'ddd' };
      });

      it('command line argument', function() {
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryName(), 'aaa_binding.node');
      });

      it('environment variable', function() {
        process.argv = [];
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryName(), 'bbb_binding.node');
      });

      it('npm config variable', function() {
        process.argv = [];
        process.env.SASS_BINARY_NAME = null;
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryName(), 'ccc_binding.node');
      });

      it('package.json', function() {
        process.argv = [];
        process.env.SASS_BINARY_NAME = null;
        process.env.npm_config_sass_binary_name = null;
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryName(), 'ddd_binding.node');
      });
    });

    describe('SASS_BINARY_SITE', function() {
      beforeEach(function() {
        process.argv.push('--sass-binary-site', 'http://aaa.example.com:9999');
        process.env.SASS_BINARY_SITE = 'http://bbb.example.com:8888';
        process.env.npm_config_sass_binary_site = 'http://ccc.example.com:7777';
        require.cache[packagePath].exports.nodeSassConfig = { binarySite: 'http://ddd.example.com:6666' };
      });

      it('command line argument', function() {
        var sass = require(extensionsPath);
        var URL = 'http://aaa.example.com:9999';
        assert.equal(sass.getBinaryUrl().substr(0, URL.length), URL);
      });

      it('environment variable', function() {
        process.argv = [];
        var sass = require(extensionsPath);
        var URL = 'http://bbb.example.com:8888';
        assert.equal(sass.getBinaryUrl().substr(0, URL.length), URL);
      });

      it('npm config variable', function() {
        process.argv = [];
        process.env.SASS_BINARY_SITE = null;
        var sass = require(extensionsPath);
        var URL = 'http://ccc.example.com:7777';
        assert.equal(sass.getBinaryUrl().substr(0, URL.length), URL);
      });

      it('package.json', function() {
        process.argv = [];
        process.env.SASS_BINARY_SITE = null;
        process.env.npm_config_sass_binary_site = null;
        var sass = require(extensionsPath);
        var URL = 'http://ddd.example.com:6666';
        assert.equal(sass.getBinaryUrl().substr(0, URL.length), URL);
      });
    });

    describe('SASS_BINARY_PATH', function() {
      beforeEach(function() {
        process.argv.push('--sass-binary-path', 'aaa_binding.node');
        process.env.SASS_BINARY_PATH = 'bbb_binding.node';
        process.env.npm_config_sass_binary_path = 'ccc_binding.node';
        require.cache[packagePath].exports.nodeSassConfig = { binaryPath: 'ddd_binding.node' };
      });

      it('command line argument', function() {
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryPath(), 'aaa_binding.node');
      });

      it('environment variable', function() {
        process.argv = [];
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryPath(), 'bbb_binding.node');
      });

      it('npm config variable', function() {
        process.argv = [];
        process.env.SASS_BINARY_PATH = null;
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryPath(), 'ccc_binding.node');
      });

      it('package.json', function() {
        process.argv = [];
        process.env.SASS_BINARY_PATH = null;
        process.env.npm_config_sass_binary_path = null;
        var sass = require(extensionsPath);
        assert.equal(sass.getBinaryPath(), 'ddd_binding.node');
      });
    });

  });
});

// describe('library detection', function() {
//   it('should throw error when libsass binary is missing.', function() {
//     var sass = require(extensionsPath),
//         originalBin = sass.getBinaryPath(),
//         renamedBin = [originalBin, '_moved'].join('');

//     assert.throws(function() {
//       fs.renameSync(originalBin, renamedBin);
//       sass.getBinaryPath(true);
//     }, /The `libsass` binding was not found/);

//     fs.renameSync(renamedBin, originalBin);
//   });
// });
