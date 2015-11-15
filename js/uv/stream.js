var sock   = require('socket');
var loop   = require('loop');
var errno  = process.binding('errno');
var uv     = require('uv');
var assert = require('assert');

var _read_mask  = (loop.POLLIN  | loop.POLLERR | loop.POLLHUP);
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

function Stream (type){
    this.type = type;
    this.accepted_fd = -1;
    this.fd  = -1;
    this.flags = 0;
    this.shutting = false;
    this.closing  = false;
    this.pending_queue = [];
    this.write_queue = [];
    this.write_queue_size = 0;
    var self = this;
    this.io_watcher = loop.io(function(h, events){
        self.stream_io(events);
    });
}

Stream.prototype.server_io = function(){
    process.errno = 0;
    while (this.fd !== -1){
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
        this.connection_cb(0);
        if (this.accepted_fd != -1) {
            throw new Error("user hasn't call this.accept");
        }
    }
};

Stream.prototype.stream_io = function(events){
    if (events === loop.POLLERR){
        throw new Error("poll error");
    }

    if (this.connect_req) {
        this.stream_connect();
        return;
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
                    //nothing to do
                    this.read_cb(0, "");
                } else {
                    this.io_watcher.remove(loop.POLLIN);
                    if (!this.io_watcher.active(loop.POLLOUT)){
                        this.close();
                    }
                }
            }
        } else {
            this.read_cb(0, buf);
            if (buf.length < MAXREAD){
                /* partial read */
            }
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
                this.write_queue_size -= n;
                if (n === length){
                    this.write_queue.shift();
                } else if (n > 0) {
                    this.write_queue[0] = buf.substring(n, length);
                }
            }
        }

        //write drain
        if (!this.write_queue.length){
            this.write_queue.length = 0;
            this.io_watcher.stop(loop.POLLOUT);
            if (this.shutting && !this.closing) { /* shutdown stream */
                var error = 0;
                if (!sock.shutdown(this.fd, sock.SHUT_WR)){
                    error = process.errno;
                }

                if (this.shutdown_cb){
                    this.shutdown_cb(error);
                }
            }
        }
    }
};

Stream.prototype.stream_eof = function(buf){
    this.eof = true;
    this.io_watcher.remove(loop.POLLIN);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.close();
    }

    this.read_cb(errno.EOF, buf);
};

Stream.prototype.shutdown = function(cb){
    if (this.shutting) return;
    this.shutdown_cb = cb;
    this.shutting = true;
    // this.io_watcher.remove(loop.POLLIN);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.io_watcher.start(this.fd, POLLWRITE);
    }
};

Stream.prototype.close = function(cb){
    this.closing = true;
    this.io_watcher.close(cb);
    
    if (this.fd !== -1){
        sock.close(this.fd);
        this.fd = -1;
    }

    if (this.accepted_fd !== -1){
        sock.close(this.accepted_fd);
        this.accepted_fd = -1;
    }

    return 0;
};

Stream.prototype.stream_open = function(fd, flags){
    if (this.keepAlive){}
    if (this.nodelay){}
    this.fd = fd;
};

//override in tcp.js
Stream.prototype.connect = function(ip, port, cb){
    return errno.EINVAL;
};

//override in tcp.js & ipc.js
Stream.prototype.listen = function(backlog, cb){
    return errno.EINVAL;
};

Stream.prototype.accept = function(client){
    client.stream_open(this.accepted_fd);
    this.accepted_fd = -1;
};

Stream.prototype.read_start = function(cb){
    this.read_cb = cb.bind(this);
    //TODO try to read immediately
    this.io_watcher.start(this.fd, POLLREAD);
};

Stream.prototype.write = function(buf){
    var self = this;
    var sockfd = self.fd;
    assert(sockfd > 0);

    var length = buf.length;
    this.write_queue_size += length;
    //try to writ the whole thing
    if (!this.write_queue.length){
        var n = sock.send(sockfd, buf, length, SEND_FLAG);
        if (n === null){
            //do nothing just yet
            //queue it for the event loop
        } else {
            this.write_queue_size -= n;
            if (n === length){
                return;
            } else if (n > 0) {
                var str = buf.substring(n, length);
                buf = str;
            }
        }
    }

    //queue to write unsuccessful written data
    //or if there is already data in write queue
    this.write_queue.push(buf);
    if (!this.io_watcher.active(loop.POLLOUT)){
        this.io_watcher.start(sockfd, POLLWRITE);
    }
};

Stream.prototype.stream_connect = function(){
    print("hi");
    //assert(stream->type == UV_TCP || stream->type == UV_NAMED_PIPE);
    //assert(req);
    var error = 0;
    if (this.delayed_error) {
        /* To smooth over the differences between unixes errors that
         * were reported synchronously on the first connect can be delayed
         * until the next tick--which is now.
         */
        error = this.delayed_error;
        this.delayed_error = 0;
    } else {
        /* Normal situation: we need to get the socket error from the kernel. */
        assert(this.fd >= 0);
        error = sock.getsockopt(this.fd,
                   sock.SOL_SOCKET,
                   sock.SO_ERROR);
    }

    if (error === errno.EINPROGRESS) {
        return;
    }

    this.connect_req = null;

    if (error || this.write_queue.length === 0) {
        this.io_watcher.stop(loop.POLLOUT);
    }

    if (this.connect_req_cb) {
        this.connect_req_cb(error);
    }

    if (this.fd === -1){
        return;
    }

    if (error) {
        //throw new Error("flush write");
        // uv__stream_flush_write_queue(stream, -ECANCELED);
        // uv__write_callbacks(stream);
    }
};

Stream.prototype.io_feed = function(){
    var io = this.io_watcher;
    setTimeout(function(){
        io.cb(io._handle, loop.POLLOUT);
    },1);
};

module.exports = Stream;
