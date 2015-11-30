var uv     = require('uv');
var errno  = process.binding('errno');
var sock   = process.binding('socket');
var ASSERT = require('assert');

var TEST_PORT = 8080;

function fail_cb() {
    ASSERT(0, "fail_cb called");
}

function write_cb(status) {
    console.log("write cb");
    this.close();
}

function connect_cb(status) {
    var tcp = this;
    ASSERT(0 === status);
    setTimeout(function(){
        tcp.write("PING", write_cb);
        ASSERT(0 == tcp.read_stop());
    }, 50);
    this.read_start(fail_cb);
}

(function(){
    var addr = uv.ip4_address("127.0.0.1", TEST_PORT);
    var tcp = new uv.TCP();
    tcp.connect(addr, connect_cb);
})();


process.on("exit", function(){

});
