var assert = require('assert').strict;
var fs = require('fs');
var path = require('path');
var sassGraph = require('../../lib/sass-graph/sass-graph');

var fixtures = path.resolve(path.join('test', 'sass-graph', 'fixtures'));

var fixture = function(name) {
  return function(file) {
    if (!file) file = 'index.scss';
    return path.join(fixtures, name, file);
  };
};

var graph = function(opts) {
  var instance, dir, isIndentedSyntax;

  function indexFile() {
    if (isIndentedSyntax) {
      return 'index.sass';
    }
    return 'index.scss';
  };

  return  {
    indented: function() {
      isIndentedSyntax = true;
      return this;
    },

    fromFixtureDir: function(name) {
      dir = fixture(name);
      instance = sassGraph.parseDir(path.dirname(dir(indexFile())), opts);
      return this;
    },

    fromFixtureFile: function(name) {
      dir = fixture(name);
      instance = sassGraph.parseFile(dir(indexFile()), opts);
      return this;
    },

    assertDecendents: function(expected) {
      var actual = [];

      instance.visitDescendents(dir(indexFile()), function(imp) {
        actual.push(imp)
      });

      assert.deepEqual(expected.map(dir), actual);
      return this;
    },

    assertAncestors: function(file, expected) {
      var actual = [];

      instance.visitAncestors(dir(file), function(imp) {
        actual.push(imp)
      });

      assert.deepEqual(expected.map(dir), actual);
      return this;
    },
  };
};

module.exports.fixture = fixture;
module.exports.graph = graph;
