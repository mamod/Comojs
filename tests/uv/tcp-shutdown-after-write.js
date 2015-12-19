var uv = require('uv');
var ASSERT = require('assert');
var TEST_PORT = 8080;

var conn;
var timer;

var connect_cb_called = 0;
var write_cb_called = 0;
var shutdown_cb_called = 0;

var conn_close_cb_called = 0;
var timer_close_cb_called = 0;


function close_cb() {
    conn_close_cb_called++;
}


function timer_cb() {

  var r = conn.write("TEST", write_cb);
  ASSERT(r == 0);

  r = conn.shutdown(shutdown_cb);
  ASSERT(r == 0);
}


function read_cb() {}


function connect_cb(status) {

  ASSERT(status == 0);
  connect_cb_called++;

  var r = conn.read_start(read_cb);
  ASSERT(r == 0);
}


function write_cb(status) {
  ASSERT(status == 0);
  write_cb_called++;
}


function shutdown_cb(status) {
  ASSERT(status == 0);
  shutdown_cb_called++;
  conn.close(close_cb);
}


(function(){

  var r;

  var addr = uv.ip4_addr("127.0.0.1", TEST_PORT);
  ASSERT(addr !== null);

  setTimeout(timer_cb, 125);

  conn = new uv.TCP();

  r = conn.connect(addr, connect_cb);
  ASSERT(r == 0);
})();

process.on('exit', function(){
  ASSERT(connect_cb_called == 1);
  ASSERT(write_cb_called == 1);
  ASSERT(shutdown_cb_called == 1);
  ASSERT(conn_close_cb_called == 1);
});
