var uv     = require('uv');
var errno  = process.binding('errno');
var sock   = process.binding('socket');
var ASSERT = require('assert');

var TEST_PORT = 8080;

var write_cb_called = 0;
var read_cb_called  = 0;

function connection_cb(status) {
    var r;

    ASSERT(status === 0);

    var tcp_peer = new uv.TCP();
    r = this.accept(tcp_peer);
    ASSERT(r === 0);
    
    tcp_peer.server = this;

    r = tcp_peer.read_start(read_cb);
    ASSERT(r == 0);

    var buf = "hello\n";

    r = tcp_peer.write(buf, write_cb);
    ASSERT(r == 0);
}


function read_cb(err, buf) {
    if (err) {
        console.log("read_cb error: %s\n", err);
        ASSERT(err === errno.ECONNRESET || err === errno.EOF);
        this.close();
        this.server.close();
    }

    read_cb_called++;
}

function connect_cb(status) {
    ASSERT(status === 0);
    /* Close the client. */
    this.close();
}

function write_cb(status) {
    ASSERT(status === 0);
    write_cb_called++;
}

(function tcp_write_to_half_open_connection () {
    var r;

    var addr = uv.ip4_address("127.0.0.1", TEST_PORT);
    ASSERT(addr !== null);

    var tcp_server = new uv.TCP();

    r = tcp_server.bind(addr, 0);
    ASSERT(r == 0);

    r = tcp_server.listen(1, connection_cb);
    ASSERT(r == 0);

    var tcp_client = new uv.TCP();
    ASSERT(r == 0);

    r = tcp_client.connect(addr, connect_cb);
    ASSERT(r == 0);
})();

process.on('exit', function(){
    ASSERT(write_cb_called > 0);
    ASSERT(read_cb_called > 0);
});
