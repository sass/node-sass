var path = require('path'),
    spawn = require('child_process').spawn,
    bin = path.join.bind(null, __dirname, '..', 'node_modules', '.bin');

/**
 * Run test suite
 *
 * @api private
 */

function suite() {
  process.env.NODESASS_COV = 1;

  var coveralls = spawn(bin('coveralls'));
  var mocha = spawn(bin('_mocha'), ['--reporter', 'mocha-lcov-reporter'], {
    env: process.env
  });

  mocha.on('error', function(err) {
    console.error(err);
    process.exit(1);
  });

  mocha.stderr.setEncoding('utf8');
  mocha.stderr.on('data', function(err) {
    console.error(err);
    process.exit(1);
  });

  mocha.stdout.pipe(coveralls.stdin);
}

/**
 * Generate coverage files
 *
 * @api private
 */

function coverage() {
  var jscoverage = spawn(bin('jscoverage'), ['lib', 'lib-cov']);

  jscoverage.on('error', function(err) {
    console.error(err);
    process.exit(1);
  });

  jscoverage.stderr.setEncoding('utf8');
  jscoverage.stderr.on('data', function(err) {
    console.error(err);
    process.exit(1);
  });

  jscoverage.on('close', suite);
}

/**
 * Run
 */

coverage();
