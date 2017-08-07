var assert = require('assert'),
  fs = require('fs-extra'),
  path = require('path'),
  temp = require('unique-temp-dir'),
  watcher = require('../lib/watcher');

describe('watcher', function() {
  var main, sibling;
  var origin = path.join(__dirname, 'fixtures', 'watcher');

  beforeEach(function() {
    var fixture = temp();
    fs.ensureDirSync(fixture);
    fs.copySync(origin, fixture);
    main = fs.realpathSync(path.join(fixture, 'main'));
    sibling = fs.realpathSync(path.join(fixture, 'sibling'));
  });

  describe('with directory', function() {
    beforeEach(function() {
      watcher.reset({
        directory: main,
        loadPaths: [main]
      });
    });

    describe('when a file is changed in the graph', function() {
      it('should return the files to compile', function() {
        var files = watcher.changed(path.join(main, 'partials', '_one.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [path.join(main, 'one.scss')],
          removed: [],
        });
      });

      describe('and @imports previously not imported files', function() {
        it('should also return the new imported files', function() {
          var file = path.join(main, 'partials', '_one.scss');
          fs.writeFileSync(file, '@import "partials/three.scss";', { flag: 'a' });
          var files = watcher.changed(file);
          assert.deepEqual(files, {
            added: [path.join(main, 'partials', '_three.scss')],
            changed: [path.join(main, 'one.scss')],
            removed: [],
          });
        });
      });
    });

    describe('when a file is changed outside the graph', function() {
      it('should return an empty array', function() {
        var files = watcher.changed(path.join(sibling, 'partials', '_three.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });

    describe('when a file is added in the graph', function() {
      it('should return only the added file', function() {
        var file = path.join(main, 'three.scss');
        fs.writeFileSync(file, '@import "paritals/two.scss";');
        var files = watcher.added(file);
        assert.deepEqual(files, {
          added: [file],
          changed: [],
          removed: [],
        });
      });

      describe('and @imports previously not imported files', function() {
        it('should also return the new imported files', function() {
          var file = path.join(main, 'three.scss');
          fs.writeFileSync(file, '@import "partials/three.scss";');
          var files = watcher.added(file);
          assert.deepEqual(files, {
            added: [
              file,
              path.join(main, 'partials', '_three.scss')
            ],
            changed: [],
            removed: [],
          });
        });
      });
    });

    describe('when a file is added outside the graph', function() {
      it('should return an empty array', function() {
        var files = watcher.added(path.join(sibling, 'three.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });

    describe('when a file is removed from the graph', function() {
      it('should return the files to compile', function() {
        var files = watcher.removed(path.join(main, 'partials', '_one.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [path.join(main, 'one.scss')],
          removed: [path.join(main, 'partials', '_one.scss')],
        });
      });
    });

    describe('when a file is removed from outside the graph', function() {
      it('should return an empty array', function() {
        var files = watcher.removed(path.join(sibling, 'partials', '_three.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });
  });

  describe('with file', function() {
    beforeEach(function() {
      watcher.reset({
        src: path.join(main, 'one.scss'),
        loadPaths: [main]
      });
    });

    describe('when a file is changed in the graph', function() {
      it('should return the files to compile', function() {
        var files = watcher.changed(path.join(main, 'partials', '_one.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [path.join(main, 'one.scss')],
          removed: [],
        });
      });
    });

    describe('when a file is changed outside the graph', function() {
      it('should return an empty array', function() {
        var files = watcher.changed(path.join(sibling, 'partials', '_three.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });

    describe('when a file is added', function() {
      it('should return an empty array', function() {
        var file = path.join(main, 'three.scss');
        fs.writeFileSync(file, '@import "paritals/one.scss";');
        var files = watcher.added(file);
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });

    describe('when a file is removed from the graph', function() {
      it('should return the files to compile', function() {
        var files = watcher.removed(path.join(main, 'partials', '_one.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [path.join(main, 'one.scss')],
          removed: [path.join(main, 'partials', '_one.scss')],
        });
      });
    });

    describe('when a file is removed from outside the graph', function() {
      it('should return an empty array', function() {
        var files = watcher.removed(path.join(sibling, 'partials', '_three.scss'));
        assert.deepEqual(files, {
          added: [],
          changed: [],
          removed: [],
        });
      });
    });
  });
});
