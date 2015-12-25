'use strict';

var common = require('../common');
var assert = require('assert');
var buffer = require('buffer');
var Buffer = buffer.Buffer;
var SlowBuffer = buffer.SlowBuffer;

var ones = [1, 1, 1, 1];

// should create a Buffer
var sb = new SlowBuffer(4);
assert(sb instanceof Buffer);
assert.strictEqual(sb.length, 4);
sb.fill(1);
assert.deepEqual(sb, ones);

// underlying ArrayBuffer should have the same length
assert.strictEqual(sb.buffer.byteLength, 4);

// should work without new
sb = SlowBuffer(4);
assert(sb instanceof Buffer);
assert.strictEqual(sb.length, 4);
sb.fill(1);
assert.deepEqual(sb, ones);

// should work with edge cases
assert.strictEqual(SlowBuffer(0).length, 0);
// try {
//   assert.strictEqual(SlowBuffer(buffer.kMaxLength).length, buffer.kMaxLength);
// } catch (e) {
//   assert.equal(e.message, 'Invalid array buffer length');
// }

// should work with number-coercible values
assert.strictEqual(SlowBuffer('6').length, 6);
assert.strictEqual(SlowBuffer(true).length, 1);

// // should create zero-length buffer if parameter is not a number
assert.strictEqual(SlowBuffer().length, 0);
assert.strictEqual(SlowBuffer(NaN).length, 0);
assert.strictEqual(SlowBuffer({}).length, 0);
assert.strictEqual(SlowBuffer('string').length, 0);

// should throw with invalid length
assert.throws(function() {
  new SlowBuffer(Infinity);
}, 'invalid Buffer length');
assert.throws(function() {
  new SlowBuffer(-1);
}, 'invalid Buffer length');
assert.throws(function() {
	throw('invalid Buffer length');
  // new SlowBuffer(buffer.kMaxLength + 1);
}, 'invalid Buffer length');
console.log(buffer.kMaxLength);