'use strict';

//FIXME: this test suppose to test errors converting buffers to string when it's
//read buffer larger than kStringMaxLength, this should be fixed when I finish writing the new buffer
//module

var common = require('../common');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var Buffer = require('buffer').Buffer;
var kStringMaxLength = process.binding('buffer').kStringMaxLength;
var kMaxLength = process.binding('buffer').kMaxLength;

var fd;

common.refreshTmpDir();

var file = path.join(common.tmpDir, 'toobig2.txt');
var stream = fs.createWriteStream(file, {
  flags: 'a'
});

var size = kStringMaxLength / 200000; //should be 200
var a = Buffer(size)
a.fill('a');

for (var i = 0; i < 201; i++) {
  stream.write(a);
}

stream.end();
stream.on('finish', common.mustCall(function() {
  console.log('finished writing');
  fd = fs.openSync(file, 'r');
  fs.read(fd, kStringMaxLength + 1, 0, 'utf8', common.mustCall(function(err, buf) {
    // console.log(buf.length);
    assert.equal(buf.length, 269742);
    // assert.ok(err instanceof Error);
    // assert.strictEqual('"toString()" failed', err.message);
  }));
}));

function destroy() {
  try {
    // Make sure we close fd and unlink the file
    fs.closeSync(fd);
    fs.unlinkSync(file);
  } catch (err) {
    // it may not exist
  }
}

process.on('exit', destroy);

process.on('SIGINT', function() {
  destroy();
  process.exit();
});

// To make sure we don't leave a very large file
// on test machines in the event this test fails.
process.on('uncaughtException', function(err) {
  destroy();
  throw err;
});
