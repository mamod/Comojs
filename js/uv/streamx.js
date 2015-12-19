var sock   = require('socket');
var loop   = require('loop');
var errno  = process.binding('errno');
var uv     = require('uv');
var assert = require('assert');


var POLLIN    = loop.POLLIN;
var POLLOUT   = loop.POLLOUT;
var POLLERR   = loop.POLLERR;
var POLLREAD  = POLLIN  | POLLERR;
var POLLWRITE = POLLOUT | POLLERR;

var _shutting_mask = ( uv.STREAM_SHUT | uv.STREAM_SHUTTING | uv.CLOSED | uv.CLOSING);

var MAXREAD   =  80 * 1024;
var SEND_FLAG = sock.MSG_NOSIGNAL ? sock.MSG_NOSIGNAL : 0;



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

    this.queued_fds      = [];
    this.pending_queue   = [];
    this.write_queue     = [];
    this.write_queue_cb  = [];
    this.write_queue_size = 0;
    
    var self = this;
    this.io_watcher = loop.io(function(h, events){
        self.stream_io(events);
    });
}

Stream.prototype.server_io = function(events){
    
    assert(events === POLLIN);
    assert(!(this.flags & uv.CLOSING));

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
            this.io_watcher.stop(POLLIN);
            return;
        }
    }
};

function _read (stream){
    var buf;
    var err = 0;
    var is_ipc;

    stream.flags &= ~uv.STREAM_READ_PARTIAL;

    /* Prevent loop starvation when the data comes in as fast as (or faster than)
     * we can read it. XXX Need to rearm fd if we switch to edge-triggered I/O.
    */
    
    var count = 32;

    var is_ipc = stream.type === "NAMED_PIPE" && stream.ipc;

    /* XXX: Maybe instead of having UV_STREAM_READING we just test if
     * tcp->read_cb is NULL or not?
    */
    
    var fd = stream.fd;
    while (stream.read_cb
          && (stream.flags & uv.STREAM_READING)
          && (count-- > 0)) {

        assert(fd >= 0);

        if (!is_ipc) {
            buf = sock.recv(fd, MAXREAD);
        } else {
            /* ipc uses recvmsg */
            throw new Error("recvmsg");
        }

        var error = process.errno;
        if (buf === null) {
            if (error === errno.EOF){
                stream.stream_eof("");
                return;
            }

            /* Error */
            if (error === errno.EAGAIN || error === errno.EWOULDBLOCK) {
                /* Wait for the next one. */
                // if (stream->flags & UV_STREAM_READING) {
                //     uv__io_start(stream->loop, &stream->io_watcher, UV__POLLIN);
                //     uv__stream_osx_interrupt_select(stream);
                // }
                stream.read_cb(0, "");
            } else {
                /* Error. User should call uv_close(). */
                stream.read_cb(error, "");
                if (stream.flags & uv.STREAM_READING) {
                    stream.flags &= ~uv.STREAM_READING;
                    stream.io_watcher.stop(POLLIN);
                    if (!stream.io_watcher.active(POLLOUT)){
                        stream.close();
                    }
                }
                return;
            }
        } else {
            /* Successful read */
            if (is_ipc) {
                throw new Error("ipc read fds");
            }

            stream.read_cb(0, buf);

            /* Return if we didn't fill the buffer, there is no more data to read. */
            if (buf.length < MAXREAD) {
                stream.flags |= uv.STREAM_READ_PARTIAL;
                return;
            }
        }
    }
}

function _write(stream) {
    
}

Stream.prototype.stream_io = function(events){
    if (events === POLLERR){
        this.maybe_error = 1;
    }

    assert(!(this.flags & uv.CLOSING));

    if (this.connect_req) {
        this.stream_connect();
        return;
    }

    var fd = this.fd;
    if ( events & POLLIN ){
        _read(this);
    }

    if (this.fd === -1) return;  /* read_cb closed stream. */

    var flag = this.flags;
    if ( (events & POLLERR) &&
      (flag & uv.STREAM_READING) &&
      (flag & uv.STREAM_READ_PARTIAL) &&
      !(flag & uv.STREAM_READ_EOF)) {

        this.stream_eof("");
    }

    if (this.fd === -1) return;  /* read_cb closed stream. */

    if (events & (POLLOUT | POLLERR)) {

        this.dowrite();

        //write drain
        if (!this.write_queue.length){
            this.write_queue.length = 0;
            this.io_watcher.stop(POLLOUT);
            var flag = this.flags;

            if ((flag & uv.STREAM_SHUTTING) && !(flag & uv.CLOSING) && !(flag & uv.STREAM_SHUT)) {
                this.flags &= ~uv.STREAM_SHUTTING;

                var error = 0;
                if (!sock.shutdown(this.fd, sock.SHUT_WR)){
                    error = process.errno;
                } else {
                    this.flags |= uv.STREAM_SHUT;
                }

                if (this.shutdown_cb){
                    this.shutdown_cb(error);
                }
            }
        }
    }
};

Stream.prototype.stream_eof = function(buf){
    this.flags |= uv.STREAM_READ_EOF;
    this.io_watcher.stop(POLLIN);
    if (!this.io_watcher.active(POLLOUT)){
        this.io_watcher.stop(POLLOUT);
    }

    this.read_cb(errno.EOF, buf);
    this.flags &= ~uv.STREAM_READING;
};

Stream.prototype.shutdown = function(cb){

    if (this.flags & _shutting_mask){
        return errno.ENOTCONN;
    }

    this.shutdown_cb = cb;
    this.flags |= uv.STREAM_SHUTTING;

    if (!this.io_watcher.active(POLLOUT)){
        this.io_watcher.start(this.fd, POLLWRITE);
    }

    return 0;
};

Stream.prototype.close = function(cb){
    
    this.flags |= uv.CLOSING;
    this.read_stop();
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

Stream.prototype.read_stop = function(cb){
    if (!(this.flags & uv.STREAM_READING)) return 0;

    this.flags &= ~uv.STREAM_READING;
    this.io_watcher.stop(POLLIN);
    if (!this.io_watcher.active(POLLOUT)){
        this.io_watcher.stop(POLLOUT);
    }

    this.read_cb = null;
    return 0;
};

Stream.prototype.stream_open = function(fd, flags){
    if (!(this.fd === -1 || this.fd === fd)){
        return errno.EBUSY;
    }

    this.flags |= flags;

    if (this.type === "TCP") {
        // if ((this.flags & uv.TCP_NODELAY) && uv__tcp_nodelay(fd, 1)){
        //     return process.errno;
        // }

        /* TODO Use delay the user passed in. */
        // if ((this.flags & uv.TCP_KEEPALIVE) && uv__tcp_keepalive(fd, 1, 60)) {
        //     return process.errno;
        // }
    }

    this.fd = fd;
    return 0;
};

//override in uv/tcp.js
Stream.prototype.connect = function(ip, port, cb){
    return errno.EINVAL;
};

//override in tcp.js & uv/ipc.js
Stream.prototype.listen = function(backlog, cb){
    return errno.EINVAL;
};

Stream.prototype.accept = function(client){
    if (this.accepted_fd === -1) return errno.EINVAL;

    client.stream_open(this.accepted_fd, uv.STREAM_READABLE | uv.STREAM_WRITABLE);
    this.accepted_fd = -1;
    return 0;
};

Stream.prototype.read_start = function(cb){
    
    if (this.flags & uv.CLOSING) return errno.EINVAL;

    if (cb) this.read_cb = cb.bind(this);
    
    this.flags |= uv.STREAM_READING;

    //TODO try to read immediately
    assert(this.fd !== -1);
    this.io_watcher.start(this.fd, POLLREAD);
    return 0;
};

Stream.prototype.dowrite = function(){
    var self = this;
    var sockfd = self.fd;
    assert(sockfd > 0);
    
    var req = this.write_queue[0];
    if (!req){
        self.write_queue.length = 0;
        return;
    }

    var buf    = req.buf;
    var length = req.len;
    

    var n = sock.send(sockfd, buf, length, SEND_FLAG);
    if (n === null){
        //do nothing just yet
        //queue it for the event loop
    } else {
        self.write_queue_size -= n;
        req.bytes += n;
        if (n === length){
            self.write_queue.shift();
            req.call(req, 0);
            return 0;
        } else if (n > 0) {
            req.buf = Buffer(buf).slice(n, length).toString();
            req.len = length - n;
            self.write_queue[0] = req;
        }
    }

    if (!self.io_watcher.active(POLLOUT)){
        self.io_watcher.start(sockfd, POLLOUT);
    }
    return n;
};

Stream.prototype.write = function(buf, cb){
    var self = this;
    var sockfd = self.fd;
    assert(sockfd > 0);

    if (typeof buf === "object"){
        var last = buf.pop();
        buf.forEach(function(b){
            self.write(b);
        });

        return self.write(last, cb);
    }

    var empty_queue = this.write_queue_size === 0;
    var length = Buffer.byteLength(buf);
    self.write_queue_size += length;

    var req = cb || function (){};
    req.buf = buf;
    req.len = length;
    req.bytes = 0;
    req.handle = self;
    
    self.write_queue.push(req);

    if (this.connect_req) {
        //still connecting
    } else if (empty_queue){
        this.dowrite();
    } else {
        self.io_watcher.start(sockfd, POLLOUT);
    }
    return 0;
};

Stream.prototype.stream_connect = function(){
    
    assert(this.type == "TCP" || this.type == "NAMED_PIPE");

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
        error = sock.getsockopt(this.fd, sock.SOL_SOCKET, sock.SO_ERROR);
    }

    if (error === errno.EINPROGRESS) {
        return;
    }

    this.connect_req = null;

    if (error || this.write_queue.length === 0) {
        this.io_watcher.stop(POLLOUT);
    }

    if (this.connect_req_cb) {
        this.connect_req_cb(error);
    }

    if (this.fd === -1){
        return;
    }

    if (error) {
        // throw new Error("flush write & connect error " + error);
        // uv__stream_flush_write_queue(stream, -ECANCELED);
        // uv__write_callbacks(stream);
    }
};

Stream.prototype.io_feed = function(){
    var io = this.io_watcher;
    process.nextTick(function(){
       io.cb(io._handle, POLLOUT);
    });
};

module.exports = Stream;
