var loop = process.binding('loop');
var assert = require('assert');

var kUnref = 1;
var kCloseCallback = 2;

function Handle(){}
Handle.wrap = function (handle, wrap){
    loop.handle_wrap(handle, wrap);
};

Handle.unwrap = function (handle){
    return loop.handle_unwrap(handle);
};

Handle.close = function (cb) {
    var wrap = this;
    if (!wrap || !wrap._handle) {
        return;
    }
    
    if (typeof cb == 'function') {
        wrap.onclose = cb;
        wrap.flags |= kCloseCallback;
    }

    loop.handle_close(wrap._handle, OnClose);
    wrap._handle = null;
}

function OnClose (handle) {
    var wrap = Handle.unwrap(handle);
    if (!wrap) throw("The wrap object should still be there");
    if (wrap._handle) throw("the handle pointer should be gone");
    if ( wrap.flags && (wrap.flags & kCloseCallback) ) {
        process.MakeCallback(wrap, 'onclose', 0);
    }
}

Handle.unref = function() {
    var wrap = this;
    if (wrap && wrap._handle) {
        loop.handle_unref(wrap._handle);
        wrap.flags |= kUnref;
    }
    return wrap;
}

Handle.ref =  function (wrap) {
    if (wrap && wrap._handle) {
        loop.handle_ref(wrap._handle);
        wrap.flags &= ~kUnref;
    }
}

module.exports = Handle;
