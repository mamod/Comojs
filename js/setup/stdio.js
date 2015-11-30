exports.createWritableStdioStream = function(fd){
    var stream;
    var tty_wrap = process.binding('tty_wrap');

    // Note stream._type is used for test-module-load-list.js

    switch (tty_wrap.guessHandleType(fd)) {
      case 'TTY':
        var tty = require('tty');
        stream = new tty.WriteStream(fd);
        stream._type = 'tty';
        break;

      case 'FILE':
        var fs = require('fs');
        stream = new fs.SyncWriteStream(fd, { autoClose: false });
        stream._type = 'fs';
        break;

      case 'PIPE':
      case 'TCP':
        var net = require('net');
        stream = new net.Socket({
          fd: fd,
          readable: false,
          writable: true
        });
        stream._type = 'pipe';
        break;

      default:
        // Probably an error on in uv_guess_handle()
        throw new Error('Implement me. Unknown stream file type!');
    }

    // For supporting legacy API we put the FD here.
    stream.fd = fd;

    stream._isStdio = true;

    return stream;
};

exports.createReadableStdioStream = function(fd){
    var tty_wrap = process.binding('tty_wrap');

    switch (tty_wrap.guessHandleType(fd)) {
    case 'TTY':
      var tty = require('tty');
      stdin = new tty.ReadStream(fd, {
        highWaterMark: 0,
        readable: true,
        writable: false
      });
      break;

    case 'FILE':
      var fs = require('fs');
      stdin = new fs.ReadStream(null, { fd: fd, autoClose: false });
      break;

    case 'PIPE':
    case 'TCP':
      var net = require('net');

      // It could be that process has been started with an IPC channel
      // sitting on fd=0, in such case the pipe for this fd is already
      // present and creating a new one will lead to the assertion failure
      // in libuv.
      if (process._channel && process._channel.fd === fd) {
        stdin = new net.Socket({
          handle: process._channel,
          readable: true,
          writable: false
        });
      } else {
        stdin = new net.Socket({
          fd: fd,
          readable: true,
          writable: false
        });
      }
      // Make sure the stdin can't be `.end()`-ed
      stdin._writableState.ended = true;
      break;

    default:
      // Probably an error on in uv_guess_handle()
      throw new Error('Implement me. Unknown stdin file type!');
    }

    // For supporting legacy API we put the FD here.
    stdin.fd = fd;

    // stdin starts out life in a paused state, but node doesn't
    // know yet.  Explicitly to readStop() it to put it in the
    // not-reading state.
    if (stdin._handle && stdin._handle.readStop) {
        stdin._handle.reading = false;
        stdin._readableState.reading = false;
        stdin._handle.readStop();
    }

    // if the user calls stdin.pause(), then we need to stop reading
    // immediately, so that the process can close down.
    stdin.on('pause', function() {
        if (!stdin._handle)
            return;
        stdin._readableState.reading = false;
        stdin._handle.reading = false;
        stdin._handle.readStop();
    });

    return stdin;
};
