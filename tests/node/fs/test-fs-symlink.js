
console.log("FIXME: need child_module to work");
process.exit(0);

'use strict';
var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');
var exec = require('child_process').exec;

var linkTime;
var fileTime;

if (common.isWindows) {
  // On Windows, creating symlinks requires admin privileges.
  // We'll only try to run symlink test if we have enough privileges.
  exec('whoami /priv', function(err, o) {
    if (err || o.indexOf('SeCreateSymbolicLinkPrivilege') == -1) {
      console.log('1..0 # Skipped: insufficient privileges');
      return;
    }
  });
}

common.refreshTmpDir();

// test creating and reading symbolic link
var linkData = path.join(common.fixturesDir, '/cycles/root.js');
var linkPath = path.join(common.tmpDir, 'symlink1.js');

fs.symlink(linkData, linkPath, function(err) {
  if (err) throw err;

  fs.lstat(linkPath, common.mustCall(function(err, stats) {
    if (err) throw err;
    linkTime = stats.mtime.getTime();
  }));

  fs.stat(linkPath, common.mustCall(function(err, stats) {
    if (err) throw err;
    fileTime = stats.mtime.getTime();
  }));

  fs.readlink(linkPath, common.mustCall(function(err, destination) {
    if (err) throw err;
    assert.equal(destination, linkData);
  }));
});


process.on('exit', function() {
  assert.notStrictEqual(linkTime, fileTime);
});
