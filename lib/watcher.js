var grapher = require('sass-graph'),
  clonedeep = require('lodash/cloneDeep'),
  path = require('path'),
  config = {},
  watcher = {},
  graph = null;

watcher.reset = function(opts) {
  config = clonedeep(opts || config || {});
  var options = {
    loadPaths: config.includePath,
    extensions: ['scss', 'sass', 'css'],
    follow: config.follow,
  };

  if (config.directory) {
    graph = grapher.parseDir(config.directory, options);
  } else {
    graph = grapher.parseFile(config.src, options);
  }

  return Object.keys(graph.index);
};

watcher.changed = function(absolutePath) {
  var files = [];

  this.reset();

  if (!absolutePath) {
    return files;
  }

  if (path.basename(absolutePath)[0] !== '_') {
    if (Object.keys(graph.index).indexOf(absolutePath) !== -1) {
      files.push(absolutePath);
    }
  }

  graph.visitAncestors(absolutePath, function(parent) {
    if (path.basename(parent)[0] !== '_') {
      files.push(parent);
    }
  });

  return files;
};

watcher.added = function(absolutePath) {
  var files = [];
  var oldIndex = Object.keys(graph.index);

  this.reset();

  if (!absolutePath) {
    return files;
  }

  var newIndex = Object.keys(graph.index);
  files.concat(newIndex.filter(function (file) {
    return oldIndex.indexOf(file) === -1;
  }));

  if (path.basename(absolutePath)[0] === '_') {
    graph.visitAncestors(absolutePath, function(parent) {
      if (path.basename(parent)[0] !== '_') {
        files.push(parent);
      }
    });
  }

  return files;
};

watcher.removed = function(absolutePath) {
  var files = [];

  graph.visitAncestors(absolutePath, function(parent) {
    if (path.basename(parent)[0] !== '_') {
      files.push(parent);
    }
  });

  this.reset();

  return files;
};

module.exports = watcher;
