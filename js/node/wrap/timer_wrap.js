var loop = process.binding('loop');
var main_loop = process.main_loop;

var Handle = process.binding('handle_wrap');

var kOnTimeout = 'ontimeout';

function Timer() {
    var handle = loop.handle_init(main_loop, OnTimeout);
    this._handle = handle;
    Handle.wrap(handle, this);
    return this;
}

Timer.kOnTimeout = kOnTimeout;

Timer.prototype.start  = function (timeout, repeat) {
    var handle = this._handle;
    loop.timer_start(handle, timeout, repeat || -1);
};

Timer.prototype.stop  = function (timeout, repeat) {
    var handle = this._handle;
    loop.timer_stop(handle);
};

Timer.prototype.close = Handle.close;
Timer.prototype.unref = Handle.unref;
Timer.prototype.ref   = Handle.ref;

Timer.now = function () {
    return loop.update_time(main_loop);
};

function OnTimeout (handle) {
    var wrap = Handle.unwrap(handle);
    process.MakeCallback(wrap, kOnTimeout);
}

module.exports = {
    Timer : Timer,
    kOnTimeout : Timer.kOnTimeout
};
