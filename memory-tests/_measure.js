'use strict';

module.exports = function iterateAndMeasure(fn, mod) {
  mod = mod || 1000000;
  var count = 0;
  // eslint-disable-next-line no-constant-condition
  while (true) {
    count++;
    fn();
    if (count % mod === 0) {
      console.log(process.memoryUsage().rss / 1000000);
    }
  }
};
