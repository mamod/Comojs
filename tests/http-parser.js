
var httpParser = require('http_parser');
var assert = require('assert');


var t = require('./http-parser/request-2.js');
var p = new httpParser(t.type);

var buf = Buffer(t.raw);

for (var i = 0; i < buf.length; i++){
    var str = Buffer(1);
    str[0] = buf[i];
    var n = p.parse(str.toString());
    assert.equal(n, 1);
}

assert.deepEqual(p.headers, t.headers);
assert.equal(p.url, t.request_url);
assert.equal(p.versionMinor(), t.http_minor);
assert.equal(p.versionMajor(), t.http_major);
assert.equal(p.method(), t.method);
assert.equal(p.shouldKeepAlive(), t.should_keep_alive);
