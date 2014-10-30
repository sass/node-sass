var fs = require('fs'),
    join = require('path').join,
    spec = join(__dirname, '..', 'fixtures', 'spec', 'spec');

/**
 * Normalize CSS
 *
 * @param {String} css
 * @api public
 */

module.exports.normalize = function(str) {
  return str.replace(/\s+/g, '').replace('{', '{\n').replace(';', ';\n');
};

/**
 * Get test suites
 *
 * @api public
 */

module.exports.getSuites = function() {
  var ret = {};
  var suites = fs.readdirSync(spec);
  var ignoreSuites = [
    'libsass-todo-issues',
    'libsass-todo-tests'
  ];

  suites.forEach(function(suite) {
    if (ignoreSuites.indexOf(suite) !== -1) {
      return;
    }

    var suitePath = join(spec, suite);
    var tests = fs.readdirSync(suitePath);

    ret[suite] = {};

    tests.forEach(function(test) {
      var testPath = join(suitePath, test);

      ret[suite][test] = {};
      ret[suite][test].src = join(testPath, 'input.scss');
      ret[suite][test].expected = join(testPath, 'expected_output.css');
      ret[suite][test].paths = [
        testPath,
        join(testPath, 'sub')
      ];
    });
  });

  return ret;
};
