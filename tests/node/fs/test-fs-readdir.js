'use strict';

var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

var readdirDir = common.tmpDir;
var files = ['empty', 'files', 'for', 'just', 'testing'];

// Make sure tmp directory is clean
common.refreshTmpDir();

// Create the necessary files
files.forEach(function(currentFile) {
  fs.closeSync(fs.openSync(readdirDir + '/' + currentFile, 'w'));
});

// Check the readdir Sync version
assert.deepEqual(files, fs.readdirSync(readdirDir).sort());

// Check the readdir async version
fs.readdir(readdirDir, common.mustCall(function(err, f) {
  assert.ifError(err);
  assert.deepEqual(files, f.sort());
}));

// readdir() on file should throw ENOTDIR
// https://github.com/joyent/node/issues/1869
assert.throws(function() {
  fs.readdirSync(__filename);
}, /ENOTDIR/);

fs.readdir(__filename, common.mustCall(function(e) {
  assert.equal(e.code, 'ENOTDIR');
}));
