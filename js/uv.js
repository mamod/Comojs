var sock   = process.binding('socket');
var sys    = process.binding('sys');
var posix  = process.binding('posix');
var isWin  = process.platform === 'win32';
var assert = require('assert');

exports.CLOSING              = 0x01;    /* uv_close() called but not finished. */
exports.CLOSED               = 0x02;    /* close(2) finished. */
exports.STREAM_READING       = 0x04;    /* uv_read_start() called. */
exports.STREAM_SHUTTING      = 0x08;    /* uv_shutdown() called but not complete. */
exports.STREAM_SHUT          = 0x10;    /* Write side closed. */
exports.STREAM_READABLE      = 0x20;    /* The stream is readable */
exports.STREAM_WRITABLE      = 0x40;    /* The stream is writable */
exports.STREAM_BLOCKING      = 0x80;    /* Synchronous writes. */
exports.STREAM_READ_PARTIAL  = 0x100;   /* read(2) read less than requested. */
exports.STREAM_READ_EOF      = 0x200;   /* read(2) read EOF. */
exports.TCP_NODELAY          = 0x400;   /* Disable Nagle. */
exports.TCP_KEEPALIVE        = 0x800;   /* Turn on keep-alive. */
exports.TCP_SINGLE_ACCEPT    = 0x1000;  /* Only accept() when idle. */
exports.HANDLE_IPV6          = 0x10000; /* Handle is bound to a IPv6 socket. */

exports.TCP_IPV6ONLY         = 1;

exports.IGNORE               = 0x00;
exports.CREATE_PIPE          = 0x01;
exports.INHERIT_FD           = 0x02;
exports.INHERIT_STREAM       = 0x04;
exports.PROCESS_SETUID       = (1 << 0);
exports.PROCESS_SETGID       = (1 << 1);
exports.PROCESS_DETACHED     = (1 << 3);
exports.PROCESS_WINDOWS_HIDE = (1 << 4);
exports.PROCESS_WINDOWS_VERBATIM_ARGUMENTS = (1 << 2);

exports.socket = function(domain, type, protocol) {
    var sockfd, err;

    if (sock.SOCK_NONBLOCK && sock.SOCK_CLOEXEC) {
        sockfd = sock.socket(domain, type | sock.SOCK_NONBLOCK | sock.SOCK_CLOEXEC, protocol);
        if (sockfd !== null) return sockfd;

        if (process.errno !== errno.EINVAL) return;
    }

    sockfd = sock.socket(domain, type, protocol);
    if (sockfd === null) return;

    if (!sock.nonblock(sockfd, 1)){
        sock.close(sockfd);
        return;
    }

    if (!sys.cloexec(sockfd, 1)){
        sock.close(sockfd);
        return;
    }

    if (sock.SO_NOSIGPIPE){
        sock.setsockopt(fd, sock.SOL_SOCKET, sock.SO_NOSIGPIPE, 1);
    }

    return sockfd;
};

exports.ip4_addr = exports.ip4_address = function(ip, port){
    return sock.pton4(ip, port);
};

exports.ip6_addr = exports.ip6_address = function(ip, port){
    return sock.pton6(ip, port);
};

exports.ip_address = function(ip, port){
    return sock.pton(ip, port);
};

exports.make_socketpair = function(fds, flags) {
    var pairs = sock.socketpair(1);

    if (!pairs) return process.errno;
    fds[0] = pairs[0];
    fds[1] = pairs[1];

    sys.cloexec(fds[0], 1);
    sys.cloexec(fds[1], 1);

    if (flags & 1) {
        sock.nonblock(fds[0], 1);
        sock.nonblock(fds[1], 1);
    }

    return 0;
};

exports.make_inheritable = function(h){
    if (!sys.cloexec(h, 0)){
        throw new Error("can't inherit handle");
    }

    return h;
};

exports.duplicate_handle = function(h){
    var dupHandle = sys.DuplicateHandle(h);
    if (!dupHandle){
        throw new Error("can't inherit handle");
    }
    return dupHandle;
};

exports.stdio_container = function(count){
    var stdio = [];
    for (var i = 0; i < count; i++){
        stdio[i] = {
            flags  : exports.IGNORE,
            stream : null,
            fd     : null
        }
    }

    return stdio;
};

exports.close = function(fd) {

    assert(fd > -1);  /* Catch uninitialized io_watcher.fd bugs. */
    // assert(fd > STDERR_FILENO);  /* Catch stdio close bugs. */

    var saved_errno = process.errno;

    if (isWin){
        rc = sys.CloseHandle(fd);
    } else {
        rc = posix.close(fd);
    }
    
    if (rc === null) {
        rc = process.errno;
        if (rc == errno.EINTR) rc = errno.EINPROGRESS;  /* For platform/libc consistency. */
        process.errno = saved_errno;
    }

    return rc;
};

var uvProcess  = require('./uv/process.js');

exports.TCP    = require('./uv/tcp.js');
exports.Pipe   = require('./uv/pipe.js');
exports.Stream = require('./uv/stream.js');
exports.TTY    = require('./uv/tty.js');
exports.spawn  = function(options){
    return new uvProcess(options);
};
