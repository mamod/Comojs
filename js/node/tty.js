'use strict';

var util = require('util');
var internalUtil = require('internal/util');
var net = require('net');
var TTY = process.binding('tty_wrap').TTY;
var isTTY = process.binding('tty_wrap').isTTY;
var inherits = util.inherits;
var errnoException = util._errnoException;


exports.isatty = function(fd) {
  return isTTY(fd);
};


// backwards-compat
exports.setRawMode = internalUtil.deprecate(function(flag) {
  if (!process.stdin.isTTY) {
    throw new Error('Can\'t set raw mode on non-tty');
  }
  process.stdin.setRawMode(flag);
}, 'tty.setRawMode is deprecated. ' +
   'Use process.stdin.setRawMode instead.');


function ReadStream(fd, options) {
  if (!(this instanceof ReadStream))
    return new ReadStream(fd, options);

  options = util._extend({
    highWaterMark: 0,
    readable: true,
    writable: false,
    handle: new TTY(fd, true)
  }, options);

  net.Socket.call(this, options);

  this.isRaw = false;
  this.isTTY = true;
}
inherits(ReadStream, net.Socket);

exports.ReadStream = ReadStream;

ReadStream.prototype.setRawMode = function(flag) {
  flag = !!flag;
  this._handle.setRawMode(flag);
  this.isRaw = flag;
};


function WriteStream(fd) {
  if (!(this instanceof WriteStream)) return new WriteStream(fd);
  net.Socket.call(this, {
    handle: new TTY(fd, false),
    readable: false,
    writable: true
  });

  var winSize = [];
  var err = this._handle.getWindowSize(winSize);
  if (!err) {
    this.columns = winSize[0];
    this.rows = winSize[1];
  }
}
inherits(WriteStream, net.Socket);
exports.WriteStream = WriteStream;


WriteStream.prototype.isTTY = true;


WriteStream.prototype._refreshSize = function() {
  var oldCols = this.columns;
  var oldRows = this.rows;
  var winSize = [];
  var err = this._handle.getWindowSize(winSize);
  if (err) {
    this.emit('error', errnoException(err, 'getWindowSize'));
    return;
  }
  var newCols = winSize[0];
  var newRows = winSize[1];
  if (oldCols !== newCols || oldRows !== newRows) {
    this.columns = newCols;
    this.rows = newRows;
    this.emit('resize');
  }
};


// backwards-compat
WriteStream.prototype.cursorTo = function(x, y) {
  require('readline').cursorTo(this, x, y);
};
WriteStream.prototype.moveCursor = function(dx, dy) {
  require('readline').moveCursor(this, dx, dy);
};
WriteStream.prototype.clearLine = function(dir) {
  require('readline').clearLine(this, dir);
};
WriteStream.prototype.clearScreenDown = function() {
  require('readline').clearScreenDown(this);
};
WriteStream.prototype.getWindowSize = function() {
  return [this.columns, this.rows];
};
