var util   = require("util");
var stream = require('./stream.js');
var sock   = require('socket');
var errno  = process.binding('errno');
var loop   = require('loop');
var uv     = require('uv');
var assert = require('assert');
var io     = process.binding('io');

var isWin  = process.platform === "win32";


util.inherits(TCP, stream);
function TCP (){
    stream.call(this, 'TCP');
}

TCP.prototype.open = function(s){
    if (!sock.nonblock(s, 1)){
        return process.errno;
    }

    return this.stream_open(s, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
}

TCP.prototype.maybe_new_socket = function(domain, flags){
    if (domain == sock.AF_UNSPEC || this.fd !== -1) {
        this.flags |= flags;
        return 0;
    }

    var fd = uv.socket(domain, sock.SOCK_STREAM, 0);
    if (!fd) return process.errno;

    this.stream_open(fd, flags);
    return 0;
};

TCP.prototype.bind = function(addr, flags) {

    var family = sock.family(addr);

    /* Cannot set IPv6-only mode on non-IPv6 socket. */
    if ((flags & uv.TCP_IPV6ONLY) && family !== uv.AF_INET6) {
        return errno.EINVAL;
    }

    var err = this.maybe_new_socket(family, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    
    if (err) return err;

    if (!sock.setsockopt(this.fd, sock.SOL_SOCKET, sock.SO_REUSEADDR, 1)) {
        return process.errno;
    }

    process.errno = 0;
    if (!sock.bind(this.fd, addr) && process.errno !== errno.EADDRINUSE) {
        if (process.errno === errno.EAFNOSUPPORT) {
            /* OSX, other BSDs and SunoS fail with EAFNOSUPPORT when binding a
            * socket created with AF_INET to an AF_INET6 address or vice versa. */
            return errno.EINVAL;
        }
        return process.errno;
    }

    this.delayed_error = process.errno;

    if (family === sock.AF_INET6) {
        this.flags |= uv.HANDLE_IPV6;
    }

    return 0;
};

TCP.prototype.listen = function(backlog, cb){
    var self = this;

    if (this.delayed_error) {
        return this.delayed_error;
    }

    this.maybe_new_socket(sock.AF_INET, uv.STREAM_READABLE);

    if (!sock.listen(this.fd, backlog)){
        return process.errno;
    }

    this.connection_cb = cb.bind(this);

    //FIXME: io_watcher still here
    //we need to free resources
    this.handle = loop.io(function(h, events){
        self.server_io(events);
    });

    this.handle.start(this.fd, loop.POLLIN);
    return 0;
};

TCP.prototype.connect = function(addr, cb){

    if (this.connect_req){
        return errno.EALREADY;  /* FIXME(bnoordhuis) -EINVAL or maybe -EBUSY. */
    }
    
    var family = sock.family(addr);

    var err = this.maybe_new_socket(family, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    if (err) return err;

    this.delayed_error = 0;

    do {
        r = sock.connect(this.fd, addr);
        if (isWin) io.can_read(this.fd, 1);
    } while (
        r === null && (process.errno === errno.EINTR ||
        isWin && process.errno === errno.EWOULDBLOCK)
    );

    var error = process.errno;
    if (r === null) {
        if (error === errno.EINPROGRESS || (isWin && error === 10056)) {
            // this.delayed_error = error;
            /* not an error */
        } else if (error === errno.ECONNREFUSED || error === 10037) {
            /* If we get a ECONNREFUSED wait until the next tick to report the
             * error. Solaris wants to report immediately--other unixes want to
             * wait.
             */
            this.delayed_error = errno.ECONNREFUSED;
        } else {
            return error;
        }
    }

    this.connect_req    = 1;
    if (cb) this.connect_req_cb = cb.bind(this);

    this.io_watcher.start(this.fd, loop.POLLOUT);

    if (this.delayed_error) {
        this.io_feed();
    }

    return 0;
};

module.exports = TCP;
