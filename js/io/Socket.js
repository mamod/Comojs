var sock   = require('socket');
var loop   = require('loop');
var errno  = process.binding('errno');

var assert    = require('assert');

var _read_mask  = (loop.POLLIN | loop.POLLERR | loop.POLLHUP);
var _write_mask = (loop.POLLOUT | loop.POLLERR | loop.POLLHUP);

var MAXREAD   = 8 * 1024;
var SEND_FLAG = sock.MSG_NOSIGNAL ? sock.MSG_NOSIGNAL : 0;
var POLLREAD  = loop.POLLIN  | loop.POLERR;
var POLLWRITE = loop.POLLOUT | loop.POLERR;

function _accept (self){
    var sockfd = self.fd;
    while (1) {
        var peerfd = sock.accept(sockfd);
        if (peerfd === null){
            if (process.errno === errno.EINTR){
                continue;
            } else {
                return;
            }
        }

        //TODO cloexec
        if (!sock.nonblock(peerfd, 1)){
            throw new Error("can't set nonblocking peerfd " + peerfd);
        }

        return peerfd;
    }
}

function Socket (options){
    options = options || {};
    this.keepAlive = options.keepAlive;
    this.nodelay  = options.nodelay;
    this.accepted_fd = -1;
    this.fd  = -1;
    this.flags = 0;
    this.stream_shutdown = false;
    this.write_queue = [];
    var self = this;
    this.io_watcher = loop.io(function(h, events){
        self.stream_io(events);
    });
}

Socket.prototype.server_io = function(){
    process.errno = 0;
    while (this.fd){
        assert(this.accepted_fd === -1);
        var ret = _accept(this);
        if (!ret){
            var err = process.errno;
            if (err === errno.EAGAIN || err === errno.EWOULDBLOCK) {
                return; //not an error
            }

            if (err === errno.ECONNABORTED){
                continue; //nothing to do
            }

            if (err === errno.EMFILE || err === errno.ENFILE) {
                throw new Error("EMFILE Error");
            }

            this.connection_cb(err);
            continue;
        }

        this.accepted_fd = ret;
        this.connection_cb();
        if (this.accepted_fd != -1) {
            throw new Error("user hasn't call this.accept");
        }
    }
};


Socket.prototype.stream_io = function(events){
    if (events === loop.POLLERR){
        throw new Error("poll error");
    }

    if (this.connection_cb){
        this.connection_cb();
        this.connection_cb = null;
    }

    var fd = this.fd;
    if ((events & _read_mask) && !this.eof ){
        var buf = sock.recv(fd, MAXREAD);
        if (buf === null){
            var err = process.errno;
            if (err === errno.EOF){
                this.stream_eof(buf);
            } else {
                if (err === errno.EWOULDBLOCK || err === errno.EAGAIN ){
                    //do nothing
                    this.read_cb(0, buf);
                } else {
                    throw new Error('error ' + err);
                }
            }
        } else {
            this.read_cb(0, buf);
            if (buf.length < MAXREAD){ /* partial read */ }
        }
    }

    if (this.fd === -1) return;  /* read_cb closed stream. */

    if (events & _write_mask) {
        var buf = this.write_queue[0];
        if (buf){
            var length = buf.length;
            var n = sock.send(this.fd, buf, length, SEND_FLAG);
            if (n === null){
                this.write_queue.length = 0;
                this.close();
                return;
            } else {
                if (n === length){
                    this.write_queue.shift();
                } else if (n > 0) {
                    this.write_queue[0] = buf.substring(n, length);
                }
            }
        }

        if (!this.write_queue.length){
            this.write_queue.length = 0;
            this.io_watcher.stop(loop.POLLOUT);
            if (this.stream_shutdown) {
                this.io_watcher.close();
                if ( this.fd === -1 ) return;
                if (!sock.close(this.fd)){
                    throw new Error("can't close fd " + this.fd);
                }
                this.fd = -1;
            }
        }
    }
};

Socket.prototype.stream_eof = function(buf){
    this.eof = true;
    this.io_watcher.stop(loop.POLLIN);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.close();
    }

    this.read_cb(errno.EOF, buf);
};

Socket.prototype.shutdown = function(){
    if (this.stream_shutdown) return;
    this.stream_shutdown = true;
    this.io_watcher.stop(loop.POLLIN);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.close();
    }
};

Socket.prototype.close = function(buf){
    //wake the event loop to close on next tick
    //we want to close after flushing all writes
    this.io_watcher.start(this.fd, POLLWRITE);
};

Socket.prototype.maybe_new = function(domain){
    if (this.fd !== -1) { return 0; }
    var fd = sock.socket(domain, sock.SOCK_STREAM, 0);
    if (!fd){
        throw new Error("can't create new socket " + process.errno);
    }
    if (sock.SO_NOSIGPIPE){
        if (!sock.setsockopt(fd, sock.SOL_SOCKET,
                      sock.SO_NOSIGPIPE, 1)){
    
            throw new Error("can't supress sigpipe " + process.errno);
        }
    }
    this.stream_open(fd);
};

Socket.prototype.stream_open = function(fd, flags){
    if (this.keepAlive){}
    if (this.nodelay){}
    this.fd = fd;
};

Socket.prototype.connect = function(ip, port, cb){
    var self = this;
    this.peeraddr = sock.pton(ip, port);
    if (this.peeraddr === null){
        throw new Error("can't create peer address " + ip);
    }

    this.connection_cb = cb.bind(this);
    this.maybe_new(sock.AF_INET);
    var conn = sock.connect(this.fd, this.peeraddr);
    if (conn === null){
        throw new Error("connection error " + process.errno);
    }

    if (!sock.nonblock(this.fd, 1)){
        throw new Error("can't set nonblocking socket " + this.fd);
    }

    this.io_watcher.start(this.fd, loop.POLLOUT | loop.POLLERR);
};

Socket.prototype.bind = function(ip, port){
    var self = this;
    this.addr = sock.pton(ip, port);
    if (this.addr === null){
        throw new Error("can't bind " + process.errno);
    }

    this.ip = ip;
    this.port = port;

    this.maybe_new(sock.AF_INET);

    if (!sock.setsockopt(this.fd, sock.SOL_SOCKET,
                      sock.SO_REUSEADDR, 1)){
    
        throw new Error("error " + process.errno);
    }

    if (!sock.bind(this.fd, this.addr)){
        throw new Error("can't bind IP address " + this.ip + " errno: " + process.errno);
    }
};

Socket.prototype.listen = function(backlog, cb){
    var self = this;
    this.maybe_new(sock.AF_INET);

    var socket = this.fd;
    if (!sock.listen(socket, backlog)){
        throw new Error('listen error ' + process.errno);
    }

    if (!sock.nonblock(socket, 1)){
        throw new Error("can't set nonblocking socket " + socket);
    }

    this.connection_cb = cb.bind(this);

    this.handle = loop.io(function(h, events){
        self.server_io(events);
    });

    this.handle.start(this.fd, loop.POLLIN);
};

Socket.prototype.accept = function(client){
    client.stream_open(this.accepted_fd);
    this.accepted_fd = -1;
};

Socket.prototype.read_start = function(cb){
    this.read_cb = cb.bind(this);
    //TODO try to read immediately
    this.io_watcher.start(this.fd, POLLREAD);
};

Socket.prototype.write = function(buf){
    var self = this;
    var sockfd = self.fd;
    assert(sockfd > 0);
    
    //try to writ the whole string
    if (!this.write_queue.length){
        var length = buf.length;
        var n = sock.send(sockfd, buf, length, SEND_FLAG);
        if (n === null){
            //do nothing just yet
            //queue it for the event loop
        } else {
            if (n === length){
                return;
            } else if (n > 0) {
                var str = buf.substring(n, length);
                buf = str;
            }
        }
    }

    //queue to write unsuccessful written data
    //or of there is already data in write queue
    this.write_queue.push(buf);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.io_watcher.start(sockfd, POLLWRITE);
    }
};

module.exports = Socket;
