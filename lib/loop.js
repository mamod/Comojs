"use strict";

var binding = process.binding('loop');
var loop_callback = {};

function Loop () {
    var loop = this;
    this.pointer = binding.default_loop();
    Loop.default_loop = Loop.default_loop || loop;
}

Loop.prototype.run = function (){
    binding.run_loop(this.pointer);
};

Loop.prototype.init_handle = function (cb){
    return binding.init_handle(this.pointer, cb);
};

Loop.prototype.timer_start = function (handle, cb, start, repeat){
    binding.timer_start(handle, cb, start, repeat);
};

Loop.prototype.io_start = function (fd, mask, cb){
    var handle = this.init_handle(cb);
    loop_callback[handle] = cb;
    binding.io_start(handle, fd, mask);
    return handle;
};

Loop.prototype.io_stop = function (handle, mask){
    binding.io_stop(handle, mask);
};

Loop.prototype.update_time = function (){
    return binding.update_time(this.pointer);
};

Loop.POLLIN  = binding.POLLIN;
Loop.POLLOUT = binding.POLLOUT;
Loop.POLLERR = binding.POLLERR;
Loop.default_loop = undefined;
module.exports = Loop;
