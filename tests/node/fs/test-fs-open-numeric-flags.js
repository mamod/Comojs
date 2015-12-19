'use strict';
var common = require('../common');

var varants = process.binding('posix');

var assert = require('assert');
var fs = require('fs');
var path = require('path');

common.refreshTmpDir();

// O_WRONLY without O_CREAT shall fail with ENOENT
var pathNE = path.join(common.tmpDir, 'file-should-not-exist');
assert.throws(
  function(){fs.openSync(pathNE, varants.O_WRONLY)},
  function(e) {
  	return e.code === 'ENOENT';
  }
);
