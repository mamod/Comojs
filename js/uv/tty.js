var util   = require("util");
var stream = require('./stream.js');
var sock   = require('socket');
var tty    = process.binding('tty');
var errno  = process.binding('errno');
var io     = process.binding('io');
var loop   = require('loop');
var uv     = require('uv');

var rd = require("readline");

util.inherits(TTY, stream);
function TTY (fd, readable){
    stream.call(this, 'TTY');

    var sockets = sock.socketpair();
    // tty.setRawMode();

    if (readable){
        rd.online = function(line){
            line += '\r\n';
            sock.send(sockets[0], line, line.length);
        };
        rd.start();
    } else {
        sock.nonblock(sockets[0], 1);
        setInterval(function(){
            if (io.can_read(sockets[0], 0) !== null) {
                var data = sock.recv(sockets[0], 10);
                if (data !== null){
                    tty.write(1, data);
                }
            }
        }, 1);
    }

    this.stream_open(sockets[1]);
}

// TTY.prototype.write = function(data){
//     if (sock.send(this.fd, data, data.length) === null){
//         throw(process.errno);
//     }
//     throw new Error("ansi write");
// };

module.exports = TTY;
