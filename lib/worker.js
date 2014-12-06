var binding = process.binding('worker');
var loop    = require('loop').default_loop;
var r       = require('module');
var assert  = require('assert');

function _toJson (data){
    return JSON.parse(data);
}

function _toString (data){
    return JSON.stringify({
        data : data
    });
}

function Worker (file, pool){
    var self = this;
    pool = pool || 1;

    var handle = loop.init_handle();
    var queue  = binding.queue_init(file, process.argv[0], pool, handle);

    self.postMessage = function (data){
        data = _toString(data);
        binding.post_message(data, queue, 0);
    };

    loop.timer_start(handle, function(h){
        while (1){
            var data = binding.watcher(queue, 0);
            if(data !== null){
                self.data = _toJson(data).data;
                self.onmessage(self);
            } else {
                break;
            }
        }
    }, 1, 1);
}

Worker.runChild = function (file){
    var e = r.require(file);
    assert(e && typeof e.onmessage === 'function');
    
    var worker;
    var queue;

    var self = {};
    self.postMessage = function(data){
        data = _toString(data);
        binding.post_message(data, queue, 1);
    };

    return function(w, q){
        worker = w;
        queue = q;
        
        var keepalive = 100;
        var count = keepalive;

        try {
            while (1){
                var data = binding.watcher(queue, 1);
                if (data !== null){
                    self.data = _toJson(data).data;
                    e.onmessage(self);
                    count = keepalive; // reset
                } else {
                    count--;
                    process.sleep(1);
                }

                if ( count === 0 ) {
                    loop.run(0);
                    break;
                } else {
                    loop.run(1);
                }
            }
        } catch (e){
            process.reportError(e);
        }
        return loop.pointer;
    };
};

module.exports = Worker;
