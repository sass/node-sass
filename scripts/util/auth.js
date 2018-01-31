var extensions = require('../../lib/extensions');
/**
 * Determine auth parameters for binaries download
 * 
 *@api private
 */
module.exports = function() {
  var auth;
  var user = extensions.getBinarySiteAuthUserName();
  var pass = extensions.getBinarySiteAuthUserPassword();

  if(user && password){
    auth = {
      username: user,
      password: pass
    }
  }

  return auth;
};
