"use strict";

module.exports = function iterateAndMeasure(fn, mod = 1000000) {
  let count = 0;
  while (true) {
    count++;
    fn();
    if (count % mod === 0) {
      console.log(process.memoryUsage().rss / 1000000);
    }
  }
}
