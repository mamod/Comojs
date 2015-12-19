var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');
var d = path.join(common.tmpDir, 'dir');

common.refreshTmpDir();

// Make sure the directory does not exist
assert(!common.fileExists(d));
// Create the directory now
fs.mkdirSync(d);
// Make sure the directory exists
assert(common.fileExists(d));
// Try creating again, it should fail with EEXIST
assert.throws(function() {
  fs.mkdirSync(d);
}, /EEXIST/);
// Remove the directory now
fs.rmdirSync(d);
// Make sure the directory does not exist
assert(!common.fileExists(d));

// Similarly test the Async version
fs.mkdir(d, 0666, function(err) {
  assert.ifError(err);

  fs.mkdir(d, 0666, function(err) {
    assert.ok(err.message.match(/EEXIST/), 'got EEXIST message');
    assert.equal(err.code, 'EEXIST', 'got EEXIST code');
    assert.equal(err.path, d, 'got proper path for EEXIST');

    fs.rmdir(d, assert.ifError);
  });
});
