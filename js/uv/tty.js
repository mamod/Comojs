var util   = require("util");
var stream   = require('./stream.js');
var sock     = require('socket');
var tty      = process.binding('tty');
var errno    = process.binding('errno');
var uv       = require('uv');
var threads  = process.binding('threads');
var posix    = process.binding('posix');

util.inherits(TTY, stream);
function TTY (){
    stream.call(this, 'TTY');
    if (uv.isWin) winTTY.apply(this, arguments);
    else nixTTY.apply(this, arguments);
}

//windows tty emulation through threads
var winTTY = function (fd, readable){

    var sockets = this.pipes = sock.socketpair();
    var self = this;

    this._fd = fd;
    sock.nonblock(sockets[0], 1);

    if (readable){
        var thread = threads.create(function(options){
            var posix   = process.bindings['posix']();
            var sock    = process.bindings['socket']();
            var threads = process.bindings['threads']();
            var tty     = process.bindings['tty']();
            var io      = process.bindings['io']();
            var line = '';

            var sockfd   = options.sockfd;
            var fd       = options.fd;

            //let parent thread notify us if this is
            //started in raw mode read
            var can_read = io.can_read(sockfd, 100);
            //consume socket buffer
            if (can_read) sock.recv(sockfd, 1, 0);
            
            while (1){
                try {
                    if (can_read){
                        var ch   = tty.read(fd);
                        if (ch < 0){
                            line = '\u001b';
                            switch(ch) {
                                case tty.KEY_DOWN  : line += '[B'; break;
                                case tty.KEY_UP    : line += '[A'; break;
                                case tty.KEY_RIGHT : line += '[C'; break;
                                case tty.KEY_LEFT  : line += '[D'; break;
                                default : throw("unknown key type");
                            }
                        } else {
                            //TODO UTF8 encoding
                            line     = Duktape.Buffer(1);
                            line[0]  = ch;
                        }
                        
                    } else {
                        var str = posix.read(fd, 1);
                        line += str;
                        if (str != '\n') continue;
                    }

                    threads.lock();
                    sock.send(sockfd, line, -1);
                    threads.unlock();
                    line = '';
                } catch(e){
                    print(e);
                    break;
                }
            }
        }, {
            sockfd : sockets[0],
            fd : fd
        });
    } else {
        setTimeout(function(){
            var tcp = this.ttyTCP = new uv.TCP();
            tcp.stream_open(sockets[0]);
            tcp.read_start(function(e, buf){
                if (e) throw new Error(e);
                threads.lock();
                tty.write(fd, buf);
                threads.unlock();
            });
        });
    }
    this.stream_open(sockets[1], readable ? uv.STREAM_READABLE : uv.STREAM_WRITABLE);
}

var nixTTY = function(fd, readable){
    var tty = this;
    var type = "TTY";
    // var type = uv.guess_handle(fd);
    // if (type == uv.FILE || type == uv.UNKNOWN_HANDLE) return errno.EINVAL;

    var flags = 0;
    var newfd;

    var skip = function(f){
        tty._fd = f;
        stream.call(tty, 'TTY');
        if (readable) flags |= uv.STREAM_READABLE;
        else flags |= uv.STREAM_WRITABLE;

        if (!(flags & uv.STREAM_BLOCKING)) uv.nonblock(f, 1);

        tty.stream_open(f, flags);
        return tty;
    };

    if (type === "TTY") {
        newfd = uv.open("/dev/tty", uv.O_RDWR);
        if (newfd === null || !uv.cloexec(newfd, 1)){
            if (readable) flags |= uv.STREAM_BLOCKING;
            return skip(fd);
        }

        var r = posix.dup2(newfd, fd);
        if (r === null && process.errno !== errno.EINVAL){
            uv.close(newfd);
            return process.errno;
        }

        uv.cloexec(newfd, 1);
        return skip(newfd);
    }
}

TTY.prototype.set_mode = function(mode){
    this.rawmode = mode;
    if (mode){
        //on windows notify thread
        //that this is a raw mode read
        if (uv.isWin) sock.send(this.fd, "1", 1);

        if (!tty.setRawMode(this._fd)){
            return process.errno;
        }
    } else {
        tty.disableRawMode(this._fd);
    }
    return 0;
}

module.exports = TTY;
