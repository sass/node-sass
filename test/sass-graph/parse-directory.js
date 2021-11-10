var assert = require('assert').strict;
var path = require('path');
var sassGraph = require('../../lib/sass-graph/sass-graph');
var graph = require('./util').graph;

describe('sass-graph', function(){
  describe('parseDir', function () {
    describe('with a simple graph', function() {
      it('should return a graph', function() {
        graph().fromFixtureDir('simple').assertDecendents([
          'b.scss',
          '_c.scss',
          path.join('nested', '_d.scss'),
          path.join('nested', '_e.scss'),
        ]);
      });
    });

    describe('with mutliple ancestors', function() {
      it('should return a graph', function() {
        graph().fromFixtureDir('mutliple-ancestors').assertAncestors('_leaf.scss', [
          'entry_a.scss',
          'entry_b.scss',
          'entry_c.scss',
          'entry_d.scss',
        ]);
      });
    });
  });
});
