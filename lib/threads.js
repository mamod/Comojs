//FIXME: nasty thread implementation

var thread = process.binding('thread');
var util = require('util');

function Threads (worker, pool) {
    pool = pool || 1;
    var self = this;
    if (!util.isNumber(pool)) {
        throw new Error('thread create second option must be a number\n' +
                        'usage : thread.create( worker (s), pool (n) )');
    }
    
    if (pool > 1) {
        self.workers = [];
        for (var i = 0; i < pool; i++){
            var w = thread.create(worker, self);
            this.workers.push(w);
        }
    } else {
        self.worker = thread.create(worker,self);
    }
    self.id = 0;
    self.data = {};
    self.cb = {};
    self.pool = pool;
}

Threads.prototype.callback = function (data, id){
    var self = this;
    var d;
    if (typeof data === 'object') {
        d = JSON.parse(data).data;
    } else {
        d = data;
    }
    
    self.cb[id](d);
    delete self.cb[id];
    delete self.data[id];
};

Threads.prototype.send = function (data, cb){
    var self = this;
    var rand = Math.floor((Math.random() * self.pool));
    var worker = self.worker || self.workers[rand];
    var send = self.send;
    var id = self.id++;
    if (typeof data === 'object') {
        data = JSON.stringify(data);
    } else {
        data = data.toString();
    }
    
    self.data[id] = data;
    self.cb[id] = cb;
    thread.send(worker, data, id);
    data = undefined;
};

exports.create = function (worker,pool){
    return new Threads(worker,pool);
};
