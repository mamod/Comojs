var binding = process.binding('worker');

var loop = process.binding('loop');
var main_loop = process.main_loop;

var r       = require('module');
var assert  = require('assert');
var id = 0;
function Worker (file){

    var self = this;
    self.id = id++;
    var onmessage = function(w, d){
        self.onmessage(w);
    };

    //initiate new worker pointer
    var worker = binding.init(main_loop, file, onmessage);

    self.destroy = function(){
        self.postMessage = function(){};
        binding.destroyWorker(worker, 1);
    };

    self.postMessage = function(data){
        binding.postMessageToChild(worker, data);
    };
}

Worker.pool = function(file, pool){
    var self = this;
    var workers = this.pool = [];
    for (var i = 0; i < pool; i++){
        this.pool[i] = new Worker(file);
    }

    this.postMessage = function(data){
        // var worker = workers[Math.floor(Math.random()*workers.length)];
        worker = workers.shift();
        if (!worker){
            worker = new Worker(file);
        }
        console.log(worker);
        worker.postMessage(data);
    };

    this.onmessage = function(){
        console.log(99);
    };
}

Worker.runChild = function (file){
    var e = r.require(file);
    var loop = process.binding('loop');
    var main_loop = process.main_loop;

    process._emitExit = function(){
        process.emit('exit');
    };

    return function callback (d, worker /* worker queue pointer */){

        if (!worker){
            loop.run(main_loop, 1);
            return callback;
        }

        var obj = {
            data : d,
            destroy : function(){
                binding.destroyWorker(worker, 1);
            },
            postMessage : function(data){
                binding.postMessageToParent(worker, data);
            }
        };

        e.onmessage(obj);
        return callback;
    };
};

module.exports = Worker;
