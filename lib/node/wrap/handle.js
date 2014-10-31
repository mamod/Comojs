var loop = require('loop').default_loop;
var kUnref = 1;
var kCloseCallback = 2;

function Handle(){}

Handle.wrap = function (handle, wrap){
    loop.handle_wrap(handle, wrap);
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

    loop.close(wrap._handle, OnClose);
    wrap._handle = null;
}

function OnClose (handle) {
    var wrap = loop.handle_unwrap(handle);
    if (!wrap) throw("The wrap object should still be there");
    if (wrap._handle) throw("the handle pointer should be gone");
    if ( wrap.flags && (wrap.flags & kCloseCallback) ) {
        MakeCallback(wrap, 'onclose');
    }
}

Handle.unref = function() {
    var wrap = this;
    if (wrap && wrap._handle) {
        loop.unref(wrap._handle);
        wrap.flags |= kUnref;
    }
    return wrap;
}

Handle.ref =  function (wrap) {
    if (wrap && wrap._handle) {
        loop.ref(wrap._handle);
        wrap.flags &= ~kUnref;
    }
}

module.exports = Handle;
