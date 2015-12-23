'use strict';

require('../common');
var assert = require('assert');


function testUint8Array(ui) {
  var length = ui.length;
  for (var i = 0; i < length; i++)
    if (ui[i] !== 0) return false;
  return true;
}


for (var i = 0; i < 100; i++) {
  new Buffer(0);
  var ui = new Uint8Array(65);
  assert.ok(testUint8Array(ui), 'Uint8Array is not zero-filled');
}
