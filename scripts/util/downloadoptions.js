var proxy = require('./proxy'),
  userAgent = require('./useragent'),
  rejectUnauthorized = require('./rejectUnauthorized');

/**
 * The options passed to make-fetch-happen when downloading the binary
 *
 * @return {Object} an options object for make-fetch-happen
 * @api private
 */
module.exports = function() {
  var options = {
    strictSSL: rejectUnauthorized(),
    timeout: 60000,
    headers: {
      'User-Agent': userAgent(),
    },
  };

  var proxyConfig = proxy();
  if (proxyConfig) {
    options.proxy = proxyConfig;
  }

  return options;
};
