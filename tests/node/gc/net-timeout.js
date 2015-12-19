'use strict';
// just like test/gc/http-client-timeout.js,
// but using a net server/client instead
var timers = require('timers');

function serverHandler(sock) {
  sock.setTimeout(120000);
  sock.resume();
  var timer;
  sock.on('close', function() {
    timers.clearTimeout(timer);
  });

  sock.on('error', function(err) {
    assert.strictEqual(err.code, 'ECONNRESET');
  });
  
  timer = timers.setTimeout(function() {
    sock.end('hello\n');
  }, 100);
}

var gc = Duktape.gc;

var net  = require('net'),
    timers = require('timers'),
    // weak    = require('weak'),
    done    = 0,
    count   = 0,
    countGC = 0,
    todo    = 250,
    common = require('../common'),
    assert = require('assert'),
    PORT = common.PORT;

console.log('We should do ' + todo + ' requests');

var server = net.createServer(serverHandler);
server.listen(PORT, getall);

function getall() {
  if (count >= todo)
    return;

  (function() {
    var req = net.connect(PORT, '127.0.0.1');
    req.resume();
    req.setTimeout(10, function() {
      //console.log('timeout (expected)')
      req.destroy();
      done++;
      gc();
    });

    count++;
    // weak(req, afterGC);
    Duktape.fin(req, function(r) {
      if (r.finalized) return;
      afterGC();
      r.finalized = true;
      // print('finalizer for f'); 
    });
  })();

  setTimeout(getall, 10);
}

for (var i = 0; i < 10; i++)
  getall();

function afterGC() {
  countGC ++;
}

timers.setInterval(status, 100);

function status() {
  gc();
  console.log('Done: %d/%d', done, todo);
  console.log('Collected: %d/%d', countGC, count);
  if (done === todo) {
    /* Give libuv some time to make close callbacks. */
    setTimeout(function() {
      gc();
      console.log('All should be collected now.');
      console.log('Collected: %d/%d', countGC, count);
      assert(count === countGC);
      process.exit(0);
    }, 200);
  }
}
