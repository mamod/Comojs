var ASSERT = require("assert");
var uv = require("uv");

var WRITES            = 3
var CHUNKS_PER_WRITE  = 409
var CHUNK_SIZE        = 1000 /* 10 kb */
var TOTAL_BYTES       =     (WRITES * CHUNKS_PER_WRITE * CHUNK_SIZE);

var shutdown_cb_called = 0;
var connect_cb_called = 0;
var write_cb_called = 0;
var close_cb_called = 0;
var bytes_sent = 0;
var bytes_sent_done = 0;
var bytes_received_done = 0;

var connect_req;
var shutdown_req;
var write_reqs = [];

var send_buffer = Buffer(CHUNK_SIZE);
send_buffer.fill('a');

function close_cb() {
    close_cb_called++;
}


function shutdown_cb(status) {
    var tcp = this;
    ASSERT(status == 0);

    /* The write buffer should be empty by now. */
    ASSERT.strictEqual(tcp.write_queue_size, 0);

    /* Now we wait for the EOF */
    shutdown_cb_called++;

    /* We should have had all the writes called already. */
    ASSERT(write_cb_called == WRITES);
}

function read_cb(e, buf) {
    var tcp = this;

    if (e) {
        ASSERT(e == uv.EOF);
        print("GOT EOF\n");
        this.close(close_cb);
    } else {
        bytes_received_done += Buffer(buf).byteLength;
        console.log(bytes_received_done);
    }
}

function write_cb(status) {
    ASSERT.equal(status, 0);
    bytes_sent_done += CHUNKS_PER_WRITE * CHUNK_SIZE;
    write_cb_called++;
}

function connect_cb(status) {
    var stream = this;

    var i, j, r;
    var send_bufs = [];
    ASSERT.equal(status, 0);
    
    connect_cb_called++;

    /* Write a lot of data */
    for (i = 0; i < WRITES; i++) {

        for (j = 0; j < CHUNKS_PER_WRITE; j++) {
            send_bufs[j] = send_buffer;
            bytes_sent += CHUNK_SIZE;
        }

        r = stream.write(send_bufs, write_cb);
        ASSERT(r == 0);
    }

    /* Shutdown on drain. */
    r = stream.shutdown(shutdown_cb);
    ASSERT(r == 0);

    /* Start reading */
    r = stream.read_start(read_cb);
    ASSERT(r == 0);
}

function tcp_writealot () {

    var addr = uv.ip4_addr("127.0.0.1", 8080);

    var client = new uv.TCP();

    r = client.connect(addr, connect_cb);
    ASSERT(r == 0);

    return 0;
}
tcp_writealot();

process.on("exit", function(){
    ASSERT(shutdown_cb_called == 1);
    ASSERT(connect_cb_called == 1);
    ASSERT(write_cb_called == WRITES);
    ASSERT(close_cb_called == 1);
    ASSERT(bytes_sent == TOTAL_BYTES);
    ASSERT(bytes_sent_done == TOTAL_BYTES);
    ASSERT.equal(bytes_received_done, TOTAL_BYTES);
});
