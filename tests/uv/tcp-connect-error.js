var uv     = require('uv');
var errno  = process.binding('errno');
var sock   = process.binding('socket');
var ASSERT = require('assert');

var connect_cb_called = 0;
var close_cb_called = 0;


(function() {
    var garbage = "blah blah blah blah blah blah blah blah blah blah blah blah";

    var garbage_addr = Duktape.Pointer({});
    var server = new uv.TCP();

    var r = server.connect(garbage_addr, function(){
        connect_cb_called++;
    });
    
    ASSERT(r == errno.EINVAL || r == errno.EAFNOSUPPORT);
    server.io_watcher.start(0);
    server.close(function(){
        close_cb_called++;
    });
})();

process.on('exit', function(){
    ASSERT(connect_cb_called == 0);
    ASSERT(close_cb_called == 1);
});
