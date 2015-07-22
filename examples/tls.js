/*  
    this is a quick demo, for demonestration only  
    you better off using io sockets or http module
    to create servers
*/

var tls    = process.binding('tls');
var path   = require('path');
var assert = require('assert');
var errno  = process.binding('errno');
var sock = require('socket');
var crt = path.resolve(__dirname + '/cert/cert.pem');
var key = path.resolve(__dirname + '/cert/key.pem');

var read = function(fd, buf, len){
    var ret = sock.readIntoBuffer(fd, buf, sock.MSG_NOSIGNAL);
    // console.log(arguments);
    return ret;
};

var write = function(fd, buf, len){
    var ret = sock.send(fd, buf, len, sock.MSG_NOSIGNAL);
    return ret;
};

var ssl = tls.init(crt, key, {
    write : write,

    read : read,

    cache : 3600
});

//SERVER
var sock = process.binding('socket');
var loop    = process.binding('loop');
var main_loop = process.main_loop;

var httpParser = require('http_parser');
var parser = new httpParser(httpParser.REQUEST);

var html =    "HTTP/1.1 200 OK\n"
        + "Content-Type: text/plain\n"
        // + "Date: Thu, 16 Jul 2016 21:34:58 GMT\n"
        + "Connection: keep-alive\n\n"
        + "Hello World";

//create new socket
var s = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
var ip = sock.pton('127.0.0.1', 9090);

if (!sock.bind(s, ip)){
    throw new Error("can't bind");
}

if (!sock.setsockopt(s, sock.SOL_SOCKET,
                      sock.SO_REUSEADDR, 1)){
    
    throw new Error("error " . process.errno);
}

if (!sock.nonblock(s, 1)){
    throw('error');
}

if (!sock.listen(s, sock.SOMAXCONN)){
    throw('listen');
}



var i = 0;
function sockcb (handle, mask) {
    //console.log(mask);
    var acceptSock = sock.accept(s);
    if (!acceptSock) {
        throw new Error("can't accept");
    }
    
    if (!sock.nonblock(acceptSock, 1)){
        throw new Error("Error Setting Socket to nonblocking");
    }


    var ctx = ssl.context(acceptSock);
    
    if (!ctx){
        sock.shutdown(acceptSock, 2);
        if (!sock.close(acceptSock)){
            throw new Error("cant close");
        }
        return;
    }

    var dir = 'handshake';
    var handle = loop.handle_init(main_loop, function(h, mask){
        if (dir === 'write') return;
        if (dir === 'handshake'){
            print("doing a handshake");
            if (ctx.handshake() === null){
                var err = process.errno;
                if (err === errno.EAGAIN || err === errno.EWOULDBLOCK) {
                    return;
                } else {

                    print("handshake failed " + process.errno);
                    sock.shutdown(acceptSock, 2);
                    if (!sock.close(acceptSock)){
                        throw new Error("cant close");
                    }
                    ctx.close();
                    loop.handle_close(h);
                    return;
                }
            }

            dir = 'read';
            print("handshake ok!!!");
        }

        if (dir === 'read'){
            var raw = ctx.read();

            if (raw === null || raw.length === 0){
                if (process.errno === errno.EWOULDBLOCK){
                    print('would block try again');
                    return;
                }

                console.log("error " + process.errno);
                if (process.errno === errno.EPIPE){
                    throw(9);
                }
                ctx.close();
                sock.shutdown(acceptSock, 2);
                sock.close(acceptSock);

                loop.handle_close(h);
                return;
            }
            dir = 'write';
        }

        ctx.write(html);
        ctx.close();
        sock.shutdown(acceptSock, 2);
        sock.close(acceptSock);
        loop.handle_close(h);
    });

    loop.io_start(handle, acceptSock, loop.POLLIN | loop.POLLERR);
}

var handle = loop.handle_init(main_loop, sockcb);
loop.io_start(handle, s, loop.POLLIN);

console.log("listening on port 9090");
