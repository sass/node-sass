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
        includePath: [main]
      });
    });

    describe('when a file is changed', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancestors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss'),
            ]);
          });
        });

        describe('if it is not a partial', function() {
          it('should record itself as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, [
              file,
            ]);
          });
        });
      });

      describe('and is not in the graph', function() {
        describe('if it is a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(sibling, 'partials', '_three.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(sibling, 'three.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });

    describe('when a file is added', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancestors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss')
            ]);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });
      });

      describe('and is not in the graph', function() {
        describe('if it is a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(sibling, 'partials', '_three.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(sibling, 'three.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });

    describe('when a file is removed', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancestors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss'),
            ]);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });
      });

      describe('and is not in the graph', function() {
        describe('if it is a partial', function() {
          it('should record nothing', function() {
            var file = path.join(sibling, 'partials', '_three.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing', function() {
            var file = path.join(sibling, 'three.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });
  });

  describe('with file', function() {
    beforeEach(function() {
      watcher.reset({
        src: path.join(main, 'one.scss'),
        includePath: [main]
      });
    });

    describe('when a file is changed', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancenstors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss'),
            ]);
          });
        });

        describe('if it is not a partial', function() {
          it('should record itself as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, [file]);
          });
        });
      });

      describe('and it is not in the graph', function() {
        describe('if it is a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(sibling, 'partials', '_three.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record itself as changed', function() {
            var file = path.join(sibling, 'three.scss');
            var files = watcher.changed(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });

    describe('when a file is added', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancestors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss'),
            ]);
          });
        });

        /* testing that this "impossible" situation is handled gracefully */
        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });
      });

      describe('and it is not in the graph', function() {
        beforeEach(function() {
          watcher.reset({
            src: path.join(main, 'two.scss'),
            includePath: [main]
          });
        });

        describe('if it is a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'partials', '_three.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'three.scss');
            var files = watcher.added(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });

    describe('when a file is removed', function() {
      describe('and it is in the graph', function() {
        describe('if it is a partial', function() {
          it('should record its ancestors as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, [
              path.join(main, 'one.scss'),
            ]);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });
      });

      describe('and is not in the graph', function() {
        beforeEach(function() {
          watcher.reset({
            src: path.join(main, 'two.scss'),
            includePath: [main]
          });
        });

        describe('if it is a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'partials', '_one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });

        describe('if it is not a partial', function() {
          it('should record nothing as changed', function() {
            var file = path.join(main, 'one.scss');
            var files = watcher.removed(file);
            assert.deepEqual(files, []);
          });
        });
      });
    });
  });
});
