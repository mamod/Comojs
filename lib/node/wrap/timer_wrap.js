var loop   = require('loop').default_loop;
var Handle = process.binding('handle_wrap');

var kOnTimeout = 0;
module.exports = {
    kOnTimeout : kOnTimeout,
    Timer : Timer
};

function Timer() {
    var handle = loop.init_handle();
    this._handle = handle;
    Handle.wrap(handle, this);
    return this;
}

Timer.prototype.start  = function (timeout, repeat) {
    var handle = this._handle;
    loop.timer_start(handle, OnTimeout, timeout, repeat);
};

Timer.prototype.close = Handle.close;
Timer.prototype.unref = Handle.unref;
Timer.prototype.ref   = Handle.ref;

Timer.now = function () {
    return loop.update_time();
};

function OnTimeout (handle) {
    var wrap = Handle.unwrap(handle);
    process.MakeCallback(wrap, kOnTimeout);
}
