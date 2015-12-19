'use strict';
var common = require('../common');
var net    = require('net');
var assert = require('assert');

var cbcount = 0;
var N = 360;

var server = net.Server(function(socket) {
  socket.on('data', function(d) {
    console.error('got %d bytes', d.length);
  });

  socket.on('end', function() {
    console.error('end');
    socket.destroy();
    server.close();
  });
});

var b = Buffer(5000);
b.fill('a');
b = b.toString();

var lastCalled = -1;
function makeCallback(c) {
  var called = false;
  return function() {
    if (called)
      throw new Error('called callback #' + c + ' more than once');
    called = true;
    if (c < lastCalled)
      throw new Error('callbacks out of order. last=' + lastCalled +
                      ' current=' + c);
    lastCalled = c;
    cbcount++;
  };
}

server.listen(common.PORT, function() {
  var client = net.createConnection(common.PORT);

  client.on('connect', function() {
    var i = 0;
    while (1) {
      i++;
      client.write(b, makeCallback(i));
      // console.log(i);
      if (i == N) break;
    }
    client.end();
  });
});

process.on('exit', function() {
  assert.equal(N, cbcount);
});
