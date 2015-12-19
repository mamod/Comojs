var uv = require('uv');
var ASSERT = require('assert');

var TEST_PORT = 8082;

var tcp_server;
var tcp_outgoing = [null, null];
var addr = null;

var tcp_incoming = [null, null];
var tcp_check;

var got_connections = 0;
var close_cb_called = 0;
var write_cb_called = 0;
var read_cb_called  = 0;

function close_cb() {
  close_cb_called++;
}

function write_cb(status) {
  ASSERT(status == 0);
  write_cb_called++;
}

function connect_cb(status) {

  if (this === tcp_check) {
    // ASSERT.notEqual(status, 0);

    /* Close check and incoming[0], time to finish test */
    tcp_incoming[0].close(close_cb);

    // for (var i = 0; i < tcp_incoming.length; i++){
    //   if (tcp_incoming[i] === this) continue;
    //   tcp_incoming[i].close(close_cb);
    // }
    
    tcp_check.close(close_cb);

    //FIXME : need to do this to close this test
    setTimeout(function(){
      process.emit('exit');
      process.exit(0);
    }, 1000);
    
    return;
  }

  ASSERT.equal(status, 0);
  var i = this._id;
  ASSERT(i >= 0);

  outgoing = tcp_outgoing[i];
  ASSERT(0 == outgoing.write("x", write_cb));
}

function read_cb(nread, buf) {
  console.log(buf);
  this._id = 3;

  /* Only first stream should receive read events */
  ASSERT(this === tcp_incoming[0]);
  ASSERT(0 == this.read_stop());
  ASSERT.equal(1, buf.length);

  read_cb_called++;

  /* Close all active incomings, except current one */
  for (var i = 0; i < got_connections; i++){
    if (tcp_incoming[i] === this) continue;
    tcp_incoming[i].close(close_cb);
  }

  /* Create new fd that should be one of the closed incomings */
  tcp_check = new uv.TCP();

  ASSERT(0 == tcp_check.connect(addr, connect_cb));

  ASSERT(0 == tcp_check.read_start(read_cb));

  /* Close server, so no one will connect to it */
  tcp_server.close(close_cb);
}

function connection_cb(status) {

  ASSERT(this === tcp_server);

  /* Ignore tcp_check connection */
  if (got_connections == tcp_incoming.length) return;

  /* Accept everyone */
  var incoming = new uv.TCP();
  tcp_incoming[got_connections++] = incoming;

  ASSERT(0 == this.accept(incoming));
  if (got_connections != tcp_incoming.length)
    return;

  /* Once all clients are accepted - start reading */
  for (var i = 0; i < tcp_incoming.length; i++) {
    incoming = tcp_incoming[i];
    ASSERT(0 == incoming.read_start(read_cb));
  }
}

(function () {

  /*
   * A little explanation of what goes on below:
   *
   * We'll create server and connect to it using two clients, each writing one
   * byte once connected.
   *
   * When all clients will be accepted by server - we'll start reading from them
   * and, on first client's first byte, will close second client and server.
   * After that, we'll immediately initiate new connection to server using
   * tcp_check handle (thus, reusing fd from second client).
   *
   * In this situation uv__io_poll()'s event list should still contain read
   * event for second client, and, if not cleaned up properly, `tcp_check` will
   * receive stale event of second incoming and invoke `connect_cb` with zero
   * status.
   */

  addr = uv.ip4_addr("127.0.0.1", TEST_PORT);
  ASSERT(addr !== null);

  tcp_server = new uv.TCP();

  ASSERT(0 == tcp_server.bind(addr, 0));
  ASSERT(0 == tcp_server.listen(tcp_outgoing.length,
                        connection_cb));

  for (var i = 0; i < tcp_outgoing.length; i++) {
    client = new uv.TCP();
    client._id = i;
    tcp_outgoing[i] = client;
    ASSERT(0 == client.connect(addr, connect_cb));
  }
})();

process.on('exit', function(){
  ASSERT.equal(tcp_outgoing.length, got_connections);
  ASSERT.equal((tcp_outgoing.length + 2), close_cb_called);
  ASSERT.equal(tcp_outgoing.length, write_cb_called);
  ASSERT.equal(1, read_cb_called);
});
