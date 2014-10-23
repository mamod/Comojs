var assert = require('../lib/node/assert.js');
Buffer.array = 1;

var cntr = 0;
var b = Buffer(1024); // safe constructor
console.log('b.length == %d', b.length);
assert.strictEqual(1024, b.length);
b[0] = -1;
assert.strictEqual(b[0], 255);
for (var i = 0; i < 1024; i++) {
b[i] = i % 256;
}
for (var i = 0; i < 1024; i++) {
assert.strictEqual(i % 256, b[i]);
}
var c = new Buffer(512);
console.log('c.length == %d', c.length);
assert.strictEqual(512, c.length);
// copy 512 bytes, from 0 to 512.
b.fill(++cntr);
c.fill(++cntr);
var copied = b.copy(c, 0, 0, 512);
console.log('copied %d bytes from b into c', copied);
assert.strictEqual(512, copied);
for (var i = 0; i < c.length; i++) {
assert.strictEqual(b[i], c[i]);
}


// copy c into b, without specifying sourceEnd
b.fill(++cntr);
c.fill(++cntr);
var copied = c.copy(b, 0, 0);
console.log('copied %d bytes from c into b w/o sourceEnd', copied);
assert.strictEqual(c.length, copied);
for (var i = 0; i < c.length; i++) {
assert.strictEqual(c[i], b[i]);
}
// copy c into b, without specifying sourceStart
b.fill(++cntr);
c.fill(++cntr);
var copied = c.copy(b, 0);
console.log('copied %d bytes from c into b w/o sourceStart', copied);
assert.strictEqual(c.length, copied);
for (var i = 0; i < c.length; i++) {
assert.strictEqual(c[i], b[i]);
}
// copy longer buffer b to shorter c without targetStart
b.fill(++cntr);
c.fill(++cntr);
var copied = b.copy(c);
console.log('copied %d bytes from b into c w/o targetStart', copied);
assert.strictEqual(c.length, copied);
for (var i = 0; i < c.length; i++) {
assert.strictEqual(b[i], c[i]);
}

// copy starting near end of b to c
b.fill(++cntr);
c.fill(++cntr);
var copied = b.copy(c, 0, b.length - Math.floor(c.length / 2));
console.log('copied %d bytes from end of b into beginning of c', copied);
assert.strictEqual(Math.floor(c.length / 2), copied);
for (var i = 0; i < Math.floor(c.length / 2); i++) {
assert.strictEqual(b[b.length - Math.floor(c.length / 2) + i], c[i]);
}
for (var i = Math.floor(c.length /2) + 1; i < c.length; i++) {
assert.strictEqual(c[c.length-1], c[i]);
}
// try to copy 513 bytes, and check we don't overrun c
b.fill(++cntr);
c.fill(++cntr);
var copied = b.copy(c, 0, 0, 513);
console.log('copied %d bytes from b trying to overrun c', copied);
assert.strictEqual(c.length, copied);
for (var i = 0; i < c.length; i++) {
assert.strictEqual(b[i], c[i]);
}
// copy 768 bytes from b into b
b.fill(++cntr);
b.fill(++cntr, 256);
var copied = b.copy(b, 0, 256, 1024);
console.log('copied %d bytes from b into b', copied);
assert.strictEqual(768, copied);
for (var i = 0; i < b.length; i++) {
assert.strictEqual(cntr, b[i]);
}
// copy string longer than buffer length (failure will segfault)
var bb = new Buffer(10);
bb.fill('hello crazy world');


var caught_error = null;
// try to copy from before the beginning of b
caught_error = null;
try {
var copied = b.copy(c, 0, 100, 10);
} catch (err) {
caught_error = err;
}
// copy throws at negative sourceStart
assert.throws(function() {
    Buffer(5).copy(Buffer(5), 0, -1);
}, RangeError);



// check sourceEnd resets to targetEnd if former is greater than the latter
b.fill(++cntr);
c.fill(++cntr);
var copied = b.copy(c, 0, 0, 1025);
console.log('copied %d bytes from b into c', copied);
for (var i = 0; i < c.length; i++) {
    assert.strictEqual(b[i], c[i]);
}

// throw with negative sourceEnd
console.log('test copy at negative sourceEnd');
assert.throws(function() {
    b.copy(c, 0, 0, -1);
}, RangeError);


// when sourceStart is greater than sourceEnd, zero copied
//assert.equal(b.copy(c, 0, 100, 10), 0);
// when targetStart > targetLength, zero copied
//assert.equal(b.copy(c, 512, 0, 10), 0);

// invalid encoding for Buffer.toString
caught_error = null;
try {
    var copied = b.toString('invalid');
} catch (err) {
    caught_error = err;
}
assert.strictEqual('Unknown encoding: invalid', caught_error.message);


// invalid encoding for Buffer.write
caught_error = null;
try {
    var copied = b.write('test string', 0, 5, 'invalid');
} catch (err) {
    caught_error = err;
}
assert.strictEqual('Unknown encoding: invalid', caught_error.message);





console.log('Try to slice off the end of the buffer');
var b = new Buffer([1, 2, 3, 4, 5]);
var b2 = b.toString('hex', 1, 10000);
var b3 = b.toString('hex', 1, 5);
var b4 = b.toString('hex', 1);
assert.equal(b2, b3);
assert.equal(b2, b4);

function buildBuffer(data) {
    if (Array.isArray(data)) {
        var buffer = new Buffer(data.length);
        data.forEach(function(v, k) {
            buffer[k] = v;
        });
        return buffer;
    }
    return null;
}
var x = buildBuffer([0x81, 0xa3, 0x66, 0x6f, 0x6f, 0xa3, 0x62, 0x61, 0x72]);
console.log(x.inspect());
assert.equal('<Buffer 81 a3 66 6f 6f a3 62 61 72>', x.inspect());

var z = x.slice(4);
console.log(z.inspect());
console.log(z.length);
assert.equal(5, z.length);
assert.equal(0x6f, z[0]);
assert.equal(0xa3, z[1]);
assert.equal(0x62, z[2]);
assert.equal(0x61, z[3]);
assert.equal(0x72, z[4]);

var z = x.slice(0);
console.log(z.inspect());
console.log(z.length);
assert.equal(z.length, x.length);
var z = x.slice(0, 4);
console.log(z.inspect());
console.log(z.length);
assert.equal(4, z.length);
assert.equal(0x81, z[0]);
assert.equal(0xa3, z[1]);
var z = x.slice(0, 9);
console.log(z.inspect());
console.log(z.length);
assert.equal(9, z.length);
var z = x.slice(1, 4);
console.log(z.inspect());
console.log(z.length);
assert.equal(3, z.length);
assert.equal(0xa3, z[0]);
var z = x.slice(2, 4);
console.log(z.inspect());
console.log(z.length);
assert.equal(2, z.length);
assert.equal(0x66, z[0]);
assert.equal(0x6f, z[1]);
assert.equal(0, Buffer('hello').slice(0, 0).length);


b = new Buffer(50);
b.fill('h');
for (var i = 0; i < b.length; i++) {
    assert.equal('h'.charCodeAt(0), b[i]);
}
b.fill(0);
for (var i = 0; i < b.length; i++) {
    assert.equal(0, b[i]);
}

b.fill(1, 16, 32);
for (var i = 0; i < 16; i++) assert.equal(0, b[i]);
for (; i < 32; i++) assert.equal(1, b[i]);
for (; i < b.length; i++) assert.equal(0, b[i]);

var buf = new Buffer(10);
buf.fill('abc');
assert.equal(buf.toString(), 'abcabcabca');
buf.fill('է');
assert.equal(buf.toString(), 'էէէէէ');

// #1210 Test UTF-8 string includes null character
var buf = new Buffer('\0');
assert.equal(buf.length, 1);
buf = new Buffer('\0\0');
assert.equal(buf.length, 2);
buf = new Buffer(2);
var written = buf.write(''); // 0byte
assert.equal(written, 0);
written = buf.write('\0'); // 1byte (v8 adds null terminator)
assert.equal(written, 1);
written = buf.write('a\0'); // 1byte * 2
assert.equal(written, 2);
written = buf.write('あ'); // 3bytes
assert.equal(written, 0);
written = buf.write('\0あ'); // 1byte + 3bytes
assert.equal(written, 1);
written = buf.write('\0\0あ'); // 1byte * 2 + 3bytes
assert.equal(written, 2);
buf = new Buffer(10);
written = buf.write('あいう'); // 3bytes * 3 (v8 adds null terminator)
assert.equal(written, 9);
written = buf.write('あいう\0'); // 3bytes * 3 + 1byte
assert.equal(written, 10);
// #243 Test write() with maxLength
var buf = new Buffer(4);
buf.fill(0xFF);
var written = buf.write('abcd', 1, 2, 'utf8');
console.log(buf);
assert.equal(written, 2);
assert.equal(buf[0], 0xFF);
assert.equal(buf[1], 0x61);
assert.equal(buf[2], 0x62);
assert.equal(buf[3], 0xFF);
buf.fill(0xFF);
written = buf.write('abcd', 1, 4);
console.log(buf);
assert.equal(written, 3);
assert.equal(buf[0], 0xFF);
assert.equal(buf[1], 0x61);
assert.equal(buf[2], 0x62);
assert.equal(buf[3], 0x63);
buf.fill(0xFF);
written = buf.write('abcd', 'utf8', 1, 2); // legacy style
console.log(buf);
assert.equal(written, 2);
assert.equal(buf[0], 0xFF);
assert.equal(buf[1], 0x61);
assert.equal(buf[2], 0x62);
assert.equal(buf[3], 0xFF);
buf.fill(0xFF);
written = buf.write('abcdef', 1, 2, 'hex');
console.log(buf);
assert.equal(written, 2);
assert.equal(buf[0], 0xFF);
assert.equal(buf[1], 0xAB);
assert.equal(buf[2], 0xCD);
assert.equal(buf[3], 0xFF);

// test unmatched surrogates not producing invalid utf8 output
// ef bf bd = utf-8 representation of unicode replacement character
// see https://codereview.chromium.org/121173009/
//buf = new Buffer('ab\ud800cd', 'utf8');
//
//assert.equal(buf[0], 0x61);
//assert.equal(buf[1], 0x62);
//assert.equal(buf[2], 0xef);
//assert.equal(buf[3], 0xbf);
//assert.equal(buf[4], 0xbd);
//assert.equal(buf[5], 0x63);
//assert.equal(buf[6], 0x64);




console.log('Done');

