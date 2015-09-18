var assert = require('assert'),
    fs = require('fs');

describe('runtime parameters', function() {
    var packagePath = require.resolve('../package'),
        extensionsPath = process.env.NODESASS_COV
          ? require.resolve('../lib-cov/extensions')
          : require.resolve('../lib/extensions'),
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
      require(extensionsPath);
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
          require(extensionsPath);
          assert.equal(process.sass.binaryName, 'aaa_binding.node');
        });

        it('environment variable', function() {
          process.argv = [];
          require(extensionsPath);
          assert.equal(process.sass.binaryName, 'bbb_binding.node');
        });

        it('npm config variable', function() {
          process.argv = [];
          process.env.SASS_BINARY_NAME = null;
          require(extensionsPath);
          assert.equal(process.sass.binaryName, 'ccc_binding.node');
        });

        it('package.json', function() {
          process.argv = [];
          process.env.SASS_BINARY_NAME = null;
          process.env.npm_config_sass_binary_name = null;
          require(extensionsPath);
          assert.equal(process.sass.binaryName, 'ddd_binding.node');
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
          require(extensionsPath);
          var URL = 'http://aaa.example.com:9999';
          assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
        });

        it('environment variable', function() {
          process.argv = [];
          require(extensionsPath);
          var URL = 'http://bbb.example.com:8888';
          assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
        });

        it('npm config variable', function() {
          process.argv = [];
          process.env.SASS_BINARY_SITE = null;
          require(extensionsPath);
          var URL = 'http://ccc.example.com:7777';
          assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
        });

        it('package.json', function() {
          process.argv = [];
          process.env.SASS_BINARY_SITE = null;
          process.env.npm_config_sass_binary_site = null;
          require(extensionsPath);
          var URL = 'http://ddd.example.com:6666';
          assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
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
          require(extensionsPath);
          assert.equal(process.sass.binaryPath, 'aaa_binding.node');
        });

        it('environment variable', function() {
          process.argv = [];
          require(extensionsPath);
          assert.equal(process.sass.binaryPath, 'bbb_binding.node');
        });

        it('npm config variable', function() {
          process.argv = [];
          process.env.SASS_BINARY_PATH = null;
          require(extensionsPath);
          assert.equal(process.sass.binaryPath, 'ccc_binding.node');
        });

        it('package.json', function() {
          process.argv = [];
          process.env.SASS_BINARY_PATH = null;
          process.env.npm_config_sass_binary_path = null;
          require(extensionsPath);
          assert.equal(process.sass.binaryPath, 'ddd_binding.node');
        });
      });

    });
});

describe('library detection', function() {
  it('should throw error when libsass binary is missing.', function() {
    var originalBin = process.sass.binaryPath,
        renamedBin = [originalBin, '_moved'].join('');

    assert.throws(function() {
      fs.renameSync(originalBin, renamedBin);
      process.sass.getBinaryPath(true);
    }, /The `libsass` binding was not found/);

    fs.renameSync(renamedBin, originalBin);
  });
});
