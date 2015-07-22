var fs = require('fs');
var assert = require('assert');

var fh = fs.open(__dirname + '/test.txt', 'r');

var buffer = Buffer('xyzw');

var bytes = fh.readIntoBuffer(buffer, {
    offset : 1,
    length : 20,
    position : 2
});

assert.strictEqual(bytes, 2);
assert.strictEqual(buffer.toString(), 'xcdw');

var bytes2 = fh.readIntoBuffer(buffer, {
    offset : 0,
    length : 4,
    position : null /* read from current file position */
});

assert.strictEqual(bytes2, 0);
assert.strictEqual(buffer.toString(), 'xcdw');


var bytes3 = fh.readIntoBuffer(buffer, {
    offset : 0,
    length : 4,
    position : 0 /* reset file position */
});

assert.strictEqual(bytes3, 4);
assert.strictEqual(buffer.toString(), 'abcd');
