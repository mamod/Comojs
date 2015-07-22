var worker = require('worker');
var assert = require('assert');


var i = 0;
if (process.main){

	var w = new worker(__filename);
	
	w.onmessage = function(data) {
		i++;
		assert.strictEqual(data, 'pong');
		print(data + " " + i);

		w.postMessage("ping");
	}

	w.postMessage("ping");
	process.on('exit', function(){
		assert.equal(i, 100);
	});
} else {
	exports.onmessage = function(w){
		assert.strictEqual(w.data, 'ping');
		if (i++ === 100){
			w.destroy();
		}
		print(w.data);
		w.postMessage('pong');
	};
	process.on('exit', function(){
		print(i);
		assert.equal(i, 101);
	});
}
