"use strict";
var assert = require('assert');
var binding = process.binding('loop');
var loop_callback = {};

Duktape.Pointer.prototype.io_start = function (fd, mask){
    binding.io_start(this, fd, mask);
};

Duktape.Pointer.prototype.io_stop = function (mask){
    binding.io_stop(this, mask);
};

Duktape.Pointer.prototype.io_close = function (){
    var handle = this;
    delete loop_callback[handle];
    binding.io_stop(handle, Loop.POLLIN | Loop.POLLOUT | Loop.POLLERR);
};

function Loop () {
    this.POLLIN = binding.POLLIN;
    this.POLLOUT = binding.POLLOUT;
    this.POLLERR = binding.POLLERR;

    this.pointer = binding.default_loop();
    Loop.default_loop = Loop.default_loop || this;
    return this;
}

Loop.prototype.run = function (type){
    binding.run_loop(this.pointer, type);
};

Loop.prototype.init_handle = function (cb){
    var handle = binding.init_handle(this.pointer, cb);
    loop_callback[handle] = cb;
    return handle;
};

Loop.prototype.handle_stop = function (handle){
    binding.handle_stop(handle);
};

Loop.prototype.handle_start = function (handle){
    binding.handle_start(handle);
};

Loop.prototype.timer_start = function (handle, cb, start, repeat){
    loop_callback[handle] = cb;
    binding.timer_start(handle, cb, start, repeat);
};

Loop.prototype.timer_again = function (handle){
    binding.timer_again(handle);
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

Loop.prototype.io_active = function (handle, mask){
    return binding.io_active(handle, mask);
};

Loop.prototype.update_time = function (){
    return binding.update_time(this.pointer);
};

Loop.prototype.close = function (handle, cb){
    binding.close(handle, cb);
};

Loop.prototype.unref = function (handle){
    binding.unref(handle);
};

Loop.prototype.ref = function (handle){
    binding.ref(handle);
};

Loop.prototype.POLLIN = Loop.POLLIN  = binding.POLLIN;
Loop.prototype.POLLOUT = Loop.POLLOUT = binding.POLLOUT;
Loop.POLLERR = binding.POLLERR;
Loop.POLLHUP = binding.POLLERR;

Loop.default_loop = undefined;
module.exports = Loop;
