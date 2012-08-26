var spawn = require('child_process').spawn;

if (process.platform === 'darwin') {
  spawn('node-gyp', ['rebuild']);
}
