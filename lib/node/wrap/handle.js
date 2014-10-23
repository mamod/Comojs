var kUnref = 1;
var kCloseCallback = 2;
var loop =  require('loop').default_loop;

module.exports = {
    ref : ref,
    unref : unref,
    close : close
};

function close (cb) {
    var wrap = this;
    if (!wrap || !wrap._handle) {
        return;
    }
    
    loop.close(wrap._handle, OnClose);
    delete wrap._handle;
    if (typeof cb == 'function') {
        wrap.onclose = cb;
        wrap.flags |= kCloseCallback;
    }
}

function OnClose (handle) {
    var wrap = handle._data;
    if (!wrap) throw("The wrap object should still be there");
    if (wrap._handle) throw("the handle pointer should be gone");
    if ( wrap.flags && (wrap.flags & kCloseCallback) ) {
        MakeCallback(wrap, 'onclose');
    }
    delete handle._data;
    wrap = undefined;
}

function unref (wrap) {
    if (wrap && wrap._handle) {
        loop.unref(wrap._handle);
        wrap.flags |= kUnref;
    }
}

function ref (wrap) {
    if (wrap && wrap._handle) {
        loop.ref(wrap._handle);
        wrap.flags &= ~kUnref;
    }
}
