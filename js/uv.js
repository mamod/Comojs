var sock = process.binding('socket');

exports.CLOSING              = 0x01,   /* uv_close() called but not finished. */
exports.CLOSED               = 0x02,   /* close(2) finished. */
exports.STREAM_READING       = 0x04,   /* uv_read_start() called. */
exports.STREAM_SHUTTING      = 0x08,   /* uv_shutdown() called but not complete. */
exports.STREAM_SHUT          = 0x10,   /* Write side closed. */
exports.STREAM_READABLE      = 0x20,   /* The stream is readable */
exports.STREAM_WRITABLE      = 0x40,   /* The stream is writable */
exports.STREAM_BLOCKING      = 0x80,   /* Synchronous writes. */
exports.STREAM_READ_PARTIAL  = 0x100,  /* read(2) read less than requested. */
exports.STREAM_READ_EOF      = 0x200,  /* read(2) read EOF. */
exports.TCP_NODELAY          = 0x400,  /* Disable Nagle. */
exports.TCP_KEEPALIVE        = 0x800,  /* Turn on keep-alive. */
exports.TCP_SINGLE_ACCEPT    = 0x1000, /* Only accept() when idle. */
exports.HANDLE_IPV6          = 0x10000 /* Handle is bound to a IPv6 socket. */

exports.ip4_address = function(ip, port){
	return sock.pton4(ip, port);
};

exports.ip6_address = function(ip, port){
	return sock.pton6(ip, port);
};

exports.ip_address = function(ip, port){
	return sock.pton(ip, port);
};

exports.TCP    = require('./uv/tcp.js');
exports.Stream = require('./uv/stream.js');
