var sock = process.binding('socket');
var binding = process.binding('io');
var loop = require('loop').default_loop;
var Stream = require('stream');

var stream = new Stream;
stream.readable = true;



var httpParser = require('http_parser');
var parser = new httpParser(httpParser.REQUEST);

var html =  "HTTP/1.1 200 OK\r\n" +
            "Content-Type: text/html; charset=utf-8\r\n" +
            "Content-Length: 8\r\n\r\n" +
            "Hi There";

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
    
    var cl = loop.io_start(acceptSock, loop.POLLIN, function(h, mask){

        var raw = sock.recv(acceptSock, 1024);
        parser.reinitialize(httpParser.REQUEST);
        parser.parse(raw);
        //console.log(parser);
        
        print(i++);

        sock.send(acceptSock, html, html.length, 0);
        sock.shutdown(acceptSock, 2);
        if (!sock.close(acceptSock)){
            throw new Error("cant close");
        }
        
        h.io_close();
    });
}

var handle = loop.io_start(s, loop.POLLIN, sockcb);
console.log("listening on port 9090");


// setInterval(function(){
//     print('running');
// },1000);
