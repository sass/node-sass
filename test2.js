var assert = require('assert');

describe('native modules from subset drive', function() {
  it('should not error', function() {
    try {
      require('s:\\node_modules\\node-sass\\vendor\\win32-ia32-48\\binding.node');
      assert.ok(true);
    } catch (e) {
      console.log(e);
      assert.ok(false);
    }
  });
});