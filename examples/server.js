/*  
    this is a quick demo, for demonestration only  
    you better off using io sockets or http module
    to create servers
*/

var sock = process.binding('socket');
var binding = process.binding('io');
var loop = process.binding('loop');
var main_loop = process.main_loop;

var httpParser = require('http_parser');
var parser = new httpParser(httpParser.REQUEST);

var html =    "HTTP/1.1 200 OK\n"
        + "Content-Type: text/plain\n"
        + "Date: Thu, 16 Jul 2015 21:34:58 GMT\n"
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

    var handle = loop.handle_init(main_loop, function(h, mask){

        var raw = sock.recv(acceptSock, 1024);
        parser.reinitialize(httpParser.REQUEST);
        parser.parse(raw);
        // console.log(parser);
        
        print(i++);

        sock.send(acceptSock, html, html.length, 0);
        sock.shutdown(acceptSock, 2);
        if (!sock.close(acceptSock)){
            throw new Error("cant close");
        }
        
        loop.handle_close(h);
    });

    loop.io_start(handle, acceptSock, loop.POLLIN);
}

var handle = loop.handle_init(main_loop, sockcb);
loop.io_start(handle, s, loop.POLLIN);

console.log("listening on port 9090 XX");


setInterval(function(){
    // print('running');
},10);
