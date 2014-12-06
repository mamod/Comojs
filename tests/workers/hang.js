//initiating a worker process
//but not sending any data to the child
//should not hang

var assert = require('assert');
var worker = require('worker');

if (process.main){
	var w =  new worker(__filename);
	var w2 = new worker(__filename);
	var w3 = new worker(__filename);
	assert.ok(1);
}
