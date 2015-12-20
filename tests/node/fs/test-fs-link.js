'use strict';
var common = require('../common');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

common.refreshTmpDir();

// test creating and reading hard link
var srcPath = path.join(common.fixturesDir, 'cycles', 'root.js');
var dstPath = path.join(common.tmpDir, 'link1.js');

var callback = function(err) {
  if (err) throw err;
  var srcContent = fs.readFileSync(srcPath, 'utf8');
  var dstContent = fs.readFileSync(dstPath, 'utf8');
  assert.strictEqual(srcContent, dstContent);
};

fs.link(srcPath, dstPath, common.mustCall(callback));

// test error outputs

assert.throws(
  function() {
    fs.link();
  },
  /src path/
);

assert.throws(
  function() {
    fs.link('abc');
  },
  /dest path/
);
