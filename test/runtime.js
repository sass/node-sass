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
        require.cache[packagePath].exports.nodeSassConfig = { binaryPath: 'bar' };
        require(extensionsPath);

        assert.equal(process.sass.binaryPath, 'bar');
      });

      it('should use the binary file name', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'foo' };
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
      });

      it('should use both the binary path and the file name', function() {
        require.cache[packagePath].exports.nodeSassConfig = { binaryName: 'foo', binaryPath: 'bar' };
        require(extensionsPath);

        assert.equal(process.sass.binaryName, 'foo_binding.node');
        assert.equal(process.sass.binaryPath, 'bar');
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
