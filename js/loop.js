var loop = process.binding('loop');
var main_loop = process.main_loop;

exports.POLLOUT = loop.POLLOUT;
exports.POLLIN  = loop.POLLIN;
exports.POLLERR = loop.POLLERR;
exports.POLLHUP = loop.POLLHUP;

/* IO HANDLE */
function IOHandle (cb){
    this.cb = cb;
    this._handle = loop.handle_init(main_loop, cb);
    this.type = loop.EV_IO;
}

IOHandle.prototype.close = function(cb){
    loop.handle_close(this._handle, cb);
};

IOHandle.prototype.stop = function(events){
    loop.io_stop(this._handle, events);
};

IOHandle.prototype.start = function(fd, events){
    loop.io_start(this._handle, fd, events);
};

IOHandle.prototype.active = function(events){
    return loop.io_active(this._handle, events);
};

IOHandle.prototype.remove = function(events){
    loop.io_remove(this._handle, events);
};

exports.io = function(cb){
    return new IOHandle(cb);
};

exports.setTimeout = setTimeout;
exports.setInterval = setInterval;
exports.clearInterval = clearInterval;
exports.clearTimeout = clearTimeout;
