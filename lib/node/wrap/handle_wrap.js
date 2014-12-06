var loop = require('loop').default_loop;
var binding = process.binding('loop');
var assert = require('assert');
var kUnref = 1;
var kCloseCallback = 2;
var HandleWrap = {};

function Handle(){}
Handle.wrap = function (handle, wrap){
    HandleWrap[handle] = wrap;
};

Handle.unwrap = function (handle){
    return HandleWrap[handle];
};

Handle.remove = function (handle){
    delete HandleWrap[handle];
};

// Handle.wrap = function (handle, wrap){
//     binding.handle_wrap(handle, wrap);
// };

// Handle.unwrap = function (handle){
//     return binding.handle_unwrap(handle);
// };

Handle.close = function (cb) {
    var wrap = this;
    if (!wrap || !wrap._handle) {
        return;
    }
    
    if (typeof cb == 'function') {
        wrap.onclose = cb;
        wrap.flags |= kCloseCallback;
    }

    //loop.close(wrap._handle, OnClose);
    wrap._handle = null;
}

function OnClose (handle) {
    var wrap = Handle.unwrap(handle);
    if (!wrap) throw("The wrap object should still be there");
    if (wrap._handle) throw("the handle pointer should be gone");
    if ( wrap.flags && (wrap.flags & kCloseCallback) ) {
        process.MakeCallback(wrap, 'onclose', 0);
    }
    
    //delete handle ref
    delete HandleWrap[handle];
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
