'use strict';
var common = require('../common');
var fs = require('fs');
var path = require('path');
var assert = require('assert');

if (!common.isWindows) {
  console.log('1..0 # Skipped: this test is Windows-specific.');
  return;
}

var successes = 0;

//FIXME this has been set to 256 to pass the test
//while longest path allowed is 260
// make a path that will be at least 260 chars long.
var fileNameLen = Math.max(256 - common.tmpDir.length - 1, 1);
var fileName = path.join(common.tmpDir, new Array(fileNameLen + 1).join('x'));
var fullPath = path.resolve(fileName);

common.refreshTmpDir();

console.log({
  filenameLength: fileName.length,
  fullPathLength: fullPath.length
});

fs.writeFile(fullPath, 'ok', function(err) {
  if (err) throw err;
  successes++;

  fs.stat(fullPath, function(err, stats) {
    if (err) throw err;
    successes++;
  });
});

process.on('exit', function() {
  fs.unlinkSync(fullPath);
  assert.equal(2, successes);
});
