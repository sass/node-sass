/*!
 * node-sass: lib/stdout.js
 */

var PARTIAL_SIZE       = 4096; // split by 4096 bytes
var WAITING_THRESHOULD = 50;

/**
 * Complete stdout writer
 *
 * @param {String} str
 */
function StdoutWriter(str) {
  this.writeData = str;
  this.offset = 0;
}

/**
 * Create partial string and write out
 */
StdoutWriter.prototype.write = function() {
  var partial = this.writeData.slice(this.offset, this.offset + PARTIAL_SIZE);
  var written = process.stdout.write(partial);

  this.offset += PARTIAL_SIZE;
  if (this.offset > this.writeData.length) {
    return;
  }
  if (written) {
    // If process.stdout.write returns true, stream wrote completely.
    // Continue to write next partial string.
    this.write();
  } else {
    // If process.stdout write returns false, stream has buffered.
    // It needs some delay untail stream has drained.
    process.stdout.once('drain', function() {
      this.write();
    }.bind(this));
  }
};

module.exports = function(str) {
  if (str instanceof Buffer) {
    str = str.toString('utf8');
  }
  var writer = new StdoutWriter(str);
  writer.write();
};

