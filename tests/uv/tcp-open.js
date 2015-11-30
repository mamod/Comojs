var uv     = require('uv');
var errno  = process.binding('errno');
var sock   = process.binding('socket');
var assert = require('assert');

var TEST_PORT = 8080;

var shutdown_cb_called = 0;
var connect_cb_called = 0;
var write_cb_called = 0;
var close_cb_called = 0;

function create_tcp_socket (){
    var s = sock.socket(sock.AF_INET, sock.SOCK_STREAM, sock.IPPROTO_IP);
    assert(s !== null);
    assert (sock.setsockopt(s, sock.SOL_SOCKET, sock.SO_REUSEADDR, 1) !== null);
    return s;
}

function shutdown_cb (){
    shutdown_cb_called++;
}

function write_cb (){
    write_cb_called++;
}

function close_cb (){
    close_cb_called++;
}

function read_cb (err, buf){
    if (!err) {
        assert(buf.length === 4);
        assert.strictEqual(buf, "PING");
    } else {

        assert(err === errno.EOF);
        print("GOT EOF\n");
        this.close(close_cb);
    }
}

function connect_cb (status){

    var buf = "PING";
    var r;


    var stream = this;
    connect_cb_called++;

    stream.write(buf, write_cb);

    /* Shutdown on drain. */
    r = stream.shutdown(shutdown_cb);
    assert(r == 0);

    /* Start reading */
    r = stream.read_start(read_cb);
    assert(r == 0);
}

(function tcp_open () {
    var r;

    var addr = uv.ip4_address("127.0.0.1", TEST_PORT);
    assert(addr !== null);

    var s = create_tcp_socket();

    assert(s !== null);

    var client = new uv.TCP();
    assert(client);

    r = client.open(s);
    assert(r === 0);

    r = client.connect(addr, connect_cb);
    assert(r === 0);
})();

process.on('exit', function(){
    assert(shutdown_cb_called === 1);
    assert(connect_cb_called == 1);
    assert(write_cb_called == 1);
    assert(close_cb_called == 1);
});
