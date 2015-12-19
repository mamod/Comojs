var uv     = require('uv');
var errno  = process.binding('errno');
var assert = require('assert');

var TEST_PORT = 8080;
(function(){
	var ip4 = uv.ip4_address("127.0.0.1", TEST_PORT);
	assert(ip4 !== null);
})();
