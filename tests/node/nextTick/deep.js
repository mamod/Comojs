'use strict';
var common = require('../common');
var assert = require('assert');

var complete = 0;

// This will fail with:
//  FATAL ERROR: JS Allocation failed - process out of memory
// if the depth counter doesn't clear the nextTickQueue properly.
(function runner() {
  if (100000 > ++complete)
    process.nextTick(runner);
}());

setTimeout(function() {
  console.log('ok');
});
