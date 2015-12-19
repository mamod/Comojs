'use strict';
var common = require('../common');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var stream = require('stream');
var encoding = 'base64';

var example = path.join(common.fixturesDir, 'x.txt');
var assertStream = new stream.Writable({
  write: function(chunk, enc, next) {
  	console.log(chunk);
    var expected = Buffer('xyz');
    assert(chunk.equals(expected));
  }
});
assertStream.setDefaultEncoding(encoding);
fs.createReadStream(example, encoding).pipe(assertStream);
