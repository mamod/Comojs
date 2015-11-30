'use strict';

var assert = require('assert');
var util = require('util');
var Socket = require('net').Socket;
var JSStream = process.binding('js_stream').JSStream;
var uv = process.binding('uv');
var debug = util.debuglog('stream_wrap');

function StreamWrap(stream) {
  var handle = new JSStream();

  this.stream = stream;

  this._list = null;

  var self = this;
  handle.close = function(cb) {
    debug('close');
    self.doClose(cb);
  };
  handle.isAlive = function() {
    return self.isAlive();
  };
  handle.isClosing = function() {
    return self.isClosing();
  };
  handle.onreadstart = function() {
    return self.readStart();
  };
  handle.onreadstop = function() {
    return self.readStop();
  };
  handle.onshutdown = function(req) {
    return self.doShutdown(req);
  };
  handle.onwrite = function(req, bufs) {
    return self.doWrite(req, bufs);
  };

  this.stream.pause();
  this.stream.on('error', function(err) {
    self.emit('error', err);
  });
  this.stream.on('data', function(chunk) {
    debug('data', chunk.length);
    if (self._handle)
      self._handle.readBuffer(chunk);
  });
  this.stream.once('end', function() {
    debug('end');
    if (self._handle)
      self._handle.emitEOF();
  });

  Socket.call(this, {
    handle: handle
  });
}
util.inherits(StreamWrap, Socket);
module.exports = StreamWrap;

// require('_stream_wrap').StreamWrap
StreamWrap.StreamWrap = StreamWrap;

StreamWrap.prototype.isAlive = function isAlive() {
  return true;
};

StreamWrap.prototype.isClosing = function isClosing() {
  return !this.readable || !this.writable;
};

StreamWrap.prototype.readStart = function readStart() {
  this.stream.resume();
  return 0;
};

StreamWrap.prototype.readStop = function readStop() {
  this.stream.pause();
  return 0;
};

StreamWrap.prototype.doShutdown = function doShutdown(req) {
  var self = this;
  var handle = this._handle;
  var item = this._enqueue('shutdown', req);

  this.stream.end(function() {
    // Ensure that write was dispatched
    setImmediate(function() {
      if (!self._dequeue(item))
        return;

      handle.finishShutdown(req, 0);
    });
  });
  return 0;
};

StreamWrap.prototype.doWrite = function doWrite(req, bufs) {
  var self = this;
  var handle = self._handle;

  var pending = bufs.length;

  // Queue the request to be able to cancel it
  var item = self._enqueue('write', req);

  self.stream.cork();
  bufs.forEach(function(buf) {
    self.stream.write(buf, done);
  });
  self.stream.uncork();

  function done(err) {
    if (!err && --pending !== 0)
      return;

    // Ensure that this is called once in case of error
    pending = 0;

    // Ensure that write was dispatched
    setImmediate(function() {
      // Do not invoke callback twice
      if (!self._dequeue(item))
        return;

      var errCode = 0;
      if (err) {
        if (err.code && uv['UV_' + err.code])
          errCode = uv['UV_' + err.code];
        else
          errCode = uv.UV_EPIPE;
      }

      handle.doAfterWrite(req);
      handle.finishWrite(req, errCode);
    });
  }

  return 0;
};

function QueueItem(type, req) {
  this.type = type;
  this.req = req;
  this.prev = this;
  this.next = this;
}

StreamWrap.prototype._enqueue = function enqueue(type, req) {
  var item = new QueueItem(type, req);
  if (this._list === null) {
    this._list = item;
    return item;
  }

  item.next = this._list.next;
  item.prev = this._list;
  item.next.prev = item;
  item.prev.next = item;

  return item;
};

StreamWrap.prototype._dequeue = function dequeue(item) {
  assert(item instanceof QueueItem);

  var next = item.next;
  var prev = item.prev;

  if (next === null && prev === null)
    return false;

  item.next = null;
  item.prev = null;

  if (next === item) {
    prev = null;
    next = null;
  } else {
    prev.next = next;
    next.prev = prev;
  }

  if (this._list === item)
    this._list = next;

  return true;
};

StreamWrap.prototype.doClose = function doClose(cb) {
  var self = this;
  var handle = self._handle;

  setImmediate(function() {
    while (self._list !== null) {
      var item = self._list;
      var req = item.req;
      self._dequeue(item);

      var errCode = uv.UV_ECANCELED;
      if (item.type === 'write') {
        handle.doAfterWrite(req);
        handle.finishWrite(req, errCode);
      } else if (item.type === 'shutdown') {
        handle.finishShutdown(req, errCode);
      }
    }

    // Should be already set by net.js
    assert(self._handle === null);
    cb();
  });
};
