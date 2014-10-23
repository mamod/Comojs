var loop = require('loop').default_loop;
var handles = {};
var kOnTimeout = 0;
module.exports = {
    kOnTimeout : kOnTimeout,
    Timer : Timer
};

function Timer () {
    var handle_ = loop.init_handle();
    handles[handle_.toString()] = this;
    this._handle = handle_;
}

Timer.prototype.start  = function (timeout, repeat) {
    var handle = this._handle;
    loop.timer_start(handle, OnTimeout, timeout, repeat);
};

Timer.prototype.close = function (){
    //print(handles[this._handle]);
    //loop.close(this._handle);
    //throw new Error("Timer close");
};

Timer.prototype.unref = function (){
    throw new Error("Timer unref");
};

Timer.prototype.ref = function (){
    throw new Error("Timer ref");
};

Timer.now = function () {
    return loop.update_time();
};

function OnTimeout (handle, status) {
    var wrap = handles[handle];
    process.MakeCallback(wrap, kOnTimeout);
}
