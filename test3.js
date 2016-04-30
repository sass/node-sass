var assert = require('assert');

describe('native modules from real drive', function() {
  it('should not error', function() {
    try {
      require('c:\\projects\\node_modules\\node-sass\\vendor\\win32-ia32-48\\binding.node');
      assert.ok(true);
    } catch (e) {
      console.log(e);
      assert.ok(false);
    }
  });
});