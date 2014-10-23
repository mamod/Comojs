var sock = process.binding('socket');
var binding = process.binding('io');
var loop = require('loop');
var default_loop = loop.default_loop;

var httpParser = require('http_parser');
var parser = new httpParser(httpParser.REQUEST);

var html =  "HTTP/1.1 200 OK\r\n" +
            "Content-Type: text/html; charset=utf-8\r\n" +
            "Content-Length: 8\r\n\r\n" +
            "Hi There";

//create new socket
var s = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 6);
var ip = sock.pton('127.0.0.1', 9090);
sock.bind(s, ip);

if (!sock.setsockopt(s, sock.SOL_SOCKET,
                      sock.SO_REUSEADDR, 1)){
    
    throw new Error("error " . process.errno);
}

sock.nonblock(s, 1);
sock.listen(s, sock.SOMAXCONN);

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
    
    var cl = default_loop.io_start(acceptSock, loop.POLLIN, function(h, mask){
        print(i++);
        var raw = sock.recv(acceptSock, 1024);
        
        parser.reinitialize(httpParser.REQUEST);
        parser.parse(raw);
        
        sock.send(acceptSock, html, html.length, 0);
        sock.shutdown(acceptSock, 2);
        if (!sock.close(acceptSock)){
            throw new Error("cant close");
        }
        
        default_loop.io_stop(cl, loop.POLLIN);
    });
}

var handle = default_loop.io_start(s, loop.POLLIN, sockcb);
console.log("listening on port 9090");
