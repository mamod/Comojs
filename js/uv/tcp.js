var util   = require("util");
var stream = require('./stream.js');
var sock   = require('socket');
var errno  = process.binding('errno');
var loop   = require('loop');
var uv     = require('uv');

util.inherits(TCP, stream);
function TCP (){
    stream.call(this, 'TCP');
}

TCP.prototype.maybe_new_socket = function(domain){
    if (this.fd !== -1) { return 0; }
    var fd = sock.socket(domain, sock.SOCK_STREAM, 0);
    if (!fd){
        return process.errno;
    }
    if (sock.SO_NOSIGPIPE){
        if (!sock.setsockopt(fd, sock.SOL_SOCKET,
                      sock.SO_NOSIGPIPE, 1)){
    
            return process.errno;
        }
    }
    this.stream_open(fd);
    return 0;
};

TCP.prototype.bind = function(ip, port){
    var self = this;
    this.addr = sock.pton(ip, port);
    if (this.addr === null){
        throw new Error("can't bind " + process.errno);
    }

    this.ip = ip;
    this.port = port;

    this.maybe_new_socket(sock.AF_INET);

    if (!sock.setsockopt(this.fd, sock.SOL_SOCKET,
                      sock.SO_REUSEADDR, 1)){
    
        throw new Error("error " + process.errno);
    }

    if (!sock.bind(this.fd, this.addr)){
        throw new Error("can't bind IP address " + this.ip + " errno: " + process.errno);
    }
};

TCP.prototype.listen = function(backlog, cb){
    var self = this;
    this.maybe_new_socket(sock.AF_INET);

    var socket = this.fd;
    if (!sock.listen(socket, backlog)){
        return process.errno;
    }

    if (!sock.nonblock(socket, 1)){
        return process.errno;
    }

    this.connection_cb = cb.bind(this);

    this.handle = loop.io(function(h, events){
        self.server_io(events);
    });

    this.handle.start(this.fd, loop.POLLIN);
    return 0;
};

TCP.prototype.connect = function(addr, cb){
    
    if (handle.connect_req){
        return errno.EALREADY;  /* FIXME(bnoordhuis) -EINVAL or maybe -EBUSY. */
    }
    
    var family = sock.family(addr);

    var err = this.maybe_new_socket(family, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    if (err) return err;

    this.delayed_error = 0;

    do {
        r = sock.connect(this.fd, addr);
    } while (r === null && process.errno === errno.EINTR);

    var error = process.errno;

    if (r === null) {
        if (error == errno.EINPROGRESS) {
            /* not an error */
        } else if (error == errno.ECONNREFUSED) {
            /* If we get a ECONNREFUSED wait until the next tick to report the
             * error. Solaris wants to report immediately--other unixes want to
             * wait.
             */
            this.delayed_error = -error;
        } else {
            return error;
        }
    }

    this.connect_req    = 1;
    this.connect_req_cb = cb.bind(this);

    this.io_watcher.start(this.fd, loop.POLLOUT);

    if (this.delayed_error) {
        this.io_feed();
    }

    return 0;
};

module.exports = TCP;
