var proxy = require('./proxy'),
  userAgent = require('./useragent');

/**
 * The options passed to request when downloading the binary
 *
 *
 * @return {Object} an options object for axios
 * @api private
 */
module.exports = function() {
  var options = {
    timeout: 60000,
    headers: {
      'User-Agent': userAgent(),
    },
    responseType: 'arraybuffer',
  };

  var proxyConfig = proxy();
  if (proxyConfig) {
    options.proxy = proxyConfig;
  }

  return options;
};
