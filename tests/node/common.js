

var path = require('path');
var fs = require('fs');
var assert = require('assert');
// var os = require('os');
// var child_process = require('child_process');
var stream = require('stream');
var util = require('util');


exports.PORT = 9999;
exports.isWindows = process.platform === 'win32';
exports.testDir = path.dirname(__filename);
exports.fixturesDir = path.join(exports.testDir, 'fixtures');
exports.tmpDirName = 'tmp';

exports.tmpDir = path.join(exports.testDir, exports.tmpDirName);

function rimrafSync(p) {
  try {
    var st = fs.lstatSync(p);
  } catch (e) {
    if (e.code === 'ENOENT')
      return;
  }

  try {
    if (st && st.isDirectory())
      rmdirSync(p, null);
    else {
      fs.unlinkSync(p);
    }
  } catch (e) {
    if (e.code === 'ENOENT')
      return;
    if (e.code === 'EPERM')
      return rmdirSync(p, e);
    if (e.code !== 'EISDIR')
      throw e;
    rmdirSync(p, e);
  }
}

function rmdirSync(p, originalEr) {
  try {
    fs.rmdirSync(p);
  } catch (e) {
    if (e.code === 'ENOTDIR')
      throw originalEr;
    if (e.code === 'ENOTEMPTY' || e.code === 'EEXIST' || e.code === 'EPERM') {
      fs.readdirSync(p).forEach(function(f) {
        rimrafSync(path.join(p, f));
      });
      fs.rmdirSync(p);
    }
  }
}

exports.refreshTmpDir = function() {
  rimrafSync(exports.tmpDir);
  try {
    fs.mkdirSync(exports.tmpDir);
  } catch(e){}
};

exports.error = console.log;

exports.fileExists = function(pathname) {
  try {
    fs.accessSync(pathname);
    return true;
  } catch (err) {
    return false;
  }
};


var mustCallChecks = [];


function runCallChecks(exitCode) {
  if (exitCode !== 0) return;

  var failed = mustCallChecks.filter(function(context) {
    return context.actual !== context.expected;
  });

  failed.forEach(function(context) {
    console.log('Mismatched %s function calls. Expected %d, actual %d.',
                context.name,
                context.expected,
                context.actual);
    console.log(context.stack.split('\n').slice(2).join('\n'));
  });

  if (failed.length) process.exit(1);
}


exports.mustCall = function(fn, expected) {
  if (typeof expected !== 'number') expected = 1;

  var context = {
    expected: expected,
    actual: 0,
    stack: (new Error()).stack,
    name: fn.name || '<anonymous>'
  };

  // add the exit listener only once to avoid listener leak warnings
  if (mustCallChecks.length === 0) process.on('exit', runCallChecks);

  mustCallChecks.push(context);

  return function() {
    context.actual++;
    return fn.apply(this, arguments);
  };
};

exports.fail = function(msg) {
  assert.fail(null, null, msg);
};
