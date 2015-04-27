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

    describe('in package.json', function() {
      it('should use the binary path', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryPath: 'ccc' };
        require(extensionsPath);

        assert.equal(process.sass.binaryPath, 'ccc');
      });

      it('should use the binary file name', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'ddd' };
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'ddd_binding.node');
      });

      it('should use both the binary path and the file name', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'foo', binaryPath: 'bar' };
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
        assert.equal(process.sass.binaryPath, 'bar');
      });

      it('should use both the binary path and the file name', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'foo', binaryPath: 'bar' };
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
        assert.equal(process.sass.binaryPath, 'bar');
      });

      it('should use the distribution site URL', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binarySite: 'http://foo.example.com:9999' };
        require(extensionsPath);

        var URL = 'http://foo.example.com:9999/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
      });

    });

    describe('in the process environment variables', function() {
      it('should use the binary path', function() {
        process.env.SASS_BINARY_PATH = 'xxx';
        require(extensionsPath);

        assert.equal(process.sass.binaryPath, 'xxx');
      });

      it('should use the binary file name', function() {
        process.env.SASS_BINARY_NAME = 'foo';
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
      });

      it('should use both the binary path and the file name', function() {
        process.env.SASS_BINARY_NAME = 'foo';
        process.env.SASS_BINARY_PATH = 'bar';
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
        assert.equal(process.sass.binaryPath, 'bar');
      });

      it('should use the distribution site URL', function() {
        process.env.SASS_BINARY_SITE = 'http://bar.example.com:9988';
        require(extensionsPath);

        var URL = 'http://bar.example.com:9988/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
      });
    });

    describe('using command line parameters', function() {
      it('should use the binary path', function() {
        process.argv.push('--sass-binary-path', 'aaa');
        require(extensionsPath);

        assert.equal(process.sass.binaryPath, 'aaa');
      });

      it('should use the binary file name', function() {
        process.argv.push('--sass-binary-name', 'bbb');
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'bbb_binding.node');
      });

      it('should use both the binary path and the file name', function() {
        process.argv.push('--sass-binary-name', 'foo', '--sass-binary-path', 'bar');
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
        assert.equal(process.sass.binaryPath, 'bar');
      });

      it('should use the distribution site URL', function() {
        process.argv.push('--sass-binary-site', 'http://qqq.example.com:9977');
        require(extensionsPath);

        var URL = 'http://qqq.example.com:9977/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
      });
    });
    describe('checking if the command line parameter overrides environment', function() {
      it('binary path', function() {
        process.argv.push('--sass-binary-path', 'bbb');
        process.env.SASS_BINARY_PATH = 'xxx';
        require(extensionsPath);
        assert.equal(process.sass.binaryPath, 'bbb');
      });
      it('binary name', function() {
        process.argv.push('--sass-binary-name', 'ccc');
        process.env.SASS_BINARY_NAME = 'yyy';
        require(extensionsPath);
        assert.equal(process.sass.binaryName, 'ccc_binding.node');
      });
      it('distribution site URL', function() {
        process.argv.push('--sass-binary-site', 'http://qqq.example.com:9977');
        process.env.SASS_BINARY_SITE = 'http://www.example.com:9988';
        require(extensionsPath);

        var URL = 'http://qqq.example.com:9977/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
      });
    });
    describe('checking if the command line parameter overrides package.json', function() {
      it('binary path', function() {
        process.argv.push('--sass-binary-path', 'ddd');
        require.cache[packagePath].exports.nodeSassConfig = { binaryPath: 'yyy' };
        require(extensionsPath);
        assert.equal(process.sass.binaryPath, 'ddd');
      });
      it('binary name', function() {
        process.argv.push('--sass-binary-name', 'eee');
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'zzz' };
        require(extensionsPath);
        assert.equal(process.sass.binaryName, 'eee_binding.node');
      });
      it('distribution site URL', function() {
        process.argv.push('--sass-binary-site', 'http://yyy.example.com:9977');
        require.cache[packagePath].exports.nodeSassConfig = { binarySite: 'http://ddd.example.com:8888' };
        require(extensionsPath);

        var URL = 'http://yyy.example.com:9977/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
      });
    });
    describe('checking if environment variable overrides package.json', function() {
      it('binary path', function() {
        process.env.SASS_BINARY_PATH = 'ggg';
        require.cache[packagePath].exports.nodeSassConfig = { binaryPath: 'qqq' };
        require(extensionsPath);
        assert.equal(process.sass.binaryPath, 'ggg');
      });
      it('binary name', function() {
        process.env.SASS_BINARY_NAME = 'hhh';
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'uuu' };
        require(extensionsPath);
        assert.equal(process.sass.binaryName, 'hhh_binding.node');
      });
      it('distribution site URL', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binarySite: 'http://ddd.example.com:8888' };
        process.env.SASS_BINARY_SITE = 'http://iii.example.com:9988';
        require(extensionsPath);

        var URL = 'http://iii.example.com:9988/v';
        assert.equal(process.sass.binaryUrl.substr(0, URL.length), URL);
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
    }, /`libsass` bindings not found/);

    fs.renameSync(renamedBin, originalBin);
  });
});
