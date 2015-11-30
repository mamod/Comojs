var util   = require("util");
var stream = require('./stream.js');
var sock   = require('socket');
var errno  = process.binding('errno');
var loop   = require('loop');
var uv     = require('uv');
var assert = require('assert');
var io     = process.binding('io');
var isWin  = process.platform === 'win32';

var pipe_map = {};

util.inherits(Pipe, stream);
function Pipe (ipc){
    stream.call(this, 'NAMED_PIPE');
    this.shutdown_req = null;
    this.connect_req = null;
    this.pipe_fname = null;
    this.ipc = ipc;
}

Pipe.prototype.open = function(fd){
    if (!sock.nonblock(fd, 1)){
        throw(process.errno);
        return process.errno;
    }

    return this.stream_open(fd, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
};

Pipe.prototype.listen = function(backlog, cb){
    var self = this;

    if (this.fd === -1) return errno.EINVAL;

    if (!sock.listen(this.fd, backlog)){
        return process.errno;
    }

    //we already have io_watcher active with
    //stream_io, clean this up and activate again
    //on server_io cb
    this.io_watcher.close();
    this.io_watcher = null;

    this.connection_cb = cb;
    this.io_watcher = loop.io(function(h, events){
        self.server_io(events);
    });

    this.io_watcher.start(this.fd, loop.POLLIN);
    return 0;
};


Pipe.prototype.bind = function(pipe_fname) {

    var sockfd;

    /* Already bound? */
    if (this.fd >= 0) {
        return errno.EINVAL;
    }

    //we are emulatiing AF_UNIX
    if (isWin){
        sockfd = uv.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
    } else {
        sockfd = uv.socket(sock.AF_UNIX, sock.SOCK_STREAM, 0);
    }

    if (!sockfd) return process.errno;
    
    var addr = sock.pton("127.0.0.1", 8000);
    if (addr === null){
        throw new Error("addr error " + process.errno);
    }

    pipe_map[pipe_fname] = addr;

    // memset(&saddr, 0, sizeof saddr);
    // strncpy(saddr.sun_path, pipe_fname, sizeof(saddr.sun_path) - 1);
    // saddr.sun_path[sizeof(saddr.sun_path) - 1] = '\0';
    // saddr.sun_family = AF_UNIX;

    if (!sock.bind(sockfd, addr)) {
        uv.close(sockfd);
        /* Convert ENOENT to EACCES for compatibility with Windows. */
        if (process.errno == errno.ENOENT) return errno.EACCES;
        return process.errno;
    }

    /* Success. */
    this.pipe_fname = pipe_fname; /* Is a strdup'ed copy. */
    this.fd = sockfd;
    return 0;
}

Pipe.prototype.connect = function(name, cb) {

    var new_sock = (this.fd === -1);

    if (new_sock) {
        if (isWin){
            newSock = uv.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
        } else {
            newSock = uv.socket(sock.AF_UNIX, sock.SOCK_STREAM, 0);
        }

        if (!newSock) return this.connect_error();
        this.fd = newSock;
    }

    // memset(&saddr, 0, sizeof saddr);
    // strncpy(saddr.sun_path, name, sizeof(saddr.sun_path) - 1);
    // saddr.sun_path[sizeof(saddr.sun_path) - 1] = '\0';
    // saddr.sun_family = AF_UNIX;

    var addr = pipe_map[name];
    var err = 0;
    var r;

    var self = this;

    this.connect_req   = 1;
    if (cb) this.connect_req_cb = cb.bind(this);

    if (new_sock) {
        err = this.stream_open(this.fd, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    }

    self.io_watcher.start(self.fd, loop.POLLIN | loop.POLLOUT | loop.POLLERR);
    sock.connect(this.fd, addr);

    this.delayed_error = err;

    // do {
    //     r = sock.connect(this.fd, addr);
    // } while ( r === null && process.errno === errno.EINTR);

    

    // if (!r && (process.errno !== errno.EINPROGRESS)) {
    //     err = process.errno;
    // } else {
    //     if (new_sock) {
    //         err = this.stream_open(this.fd, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    //     }

    //     if (err == 0) {
    //         this.io_watcher.start(loop.POLLIN | loop.POLLOUT);
    //     }
    // }

    // this.delayed_error = err;
    // this.connect_req   = 1;
    // if (cb) this.connect_req_cb = cb.bind(this);

    // /* Force callback to run on next tick in case of error. */
    // if (err) this.io_feed();
    return 0;
}

module.exports = Pipe;
