var Worker = require("../lib/worker");
var sock   = process.binding('socket');
var loop   = require('loop').default_loop;

var httpParser = require('http_parser');
var parser = new httpParser(httpParser.REQUEST);
var html =  "HTTP/1.1 200 OK\r\n" +
                "Content-Type: text/html; charset=utf-8\r\n" +
                "Content-Length: 8\r\n\r\n" +
                "Hi There";

var count = 0;

function main() {

    /* initiate a worker with maximum 20 threads pool */
    var w = new Worker(__filename, 40, {
        keepalive : 10000
    });

    /* we got message from a worker thread */
    w.onmessage = function(e) {
        print(count++);
    };

    /* we got an error from worker thread */
    w.onerror = function(e) {
        print(e.data);
    }
    
    //create new socket
    var s = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 6);
    var ip = sock.pton('127.0.0.1', 9090);
    
    if (!sock.setsockopt(s, sock.SOL_SOCKET,
                          sock.SO_REUSEADDR, 1)){
        
        throw new Error("error " . process.errno);
    }

    sock.bind(s, ip);
    sock.nonblock(s, 1);
    sock.listen(s, sock.SOMAXCONN);
    var i = 0;
    
    function sockcb (handle, mask) {
        var acceptSock = sock.accept(s);
        if (!acceptSock) {
            throw new Error("can't accept");
        }

        //we will send accepted socket to the worker
        //and back listening again, once the thread 
        w.postMessage(acceptSock);
    }

    var handle = loop.io_start(s, loop.POLLIN, sockcb);
    console.log("listening on port 9090");
}

//worker must export an onmessage function as it's starting point
exports.onmessage = function(e) {
    var acceptSock = +e.data;
    
    /* uncomment this to emulate client delayed response
    since this is a thread worker main thread will stay responsive*/
        
    //process.sleep(10);

    if (!sock.nonblock(acceptSock, 1)){
        throw new Error("Error Setting Socket to nonblocking");
    }

    var cl = loop.io_start(acceptSock, loop.POLLIN, function(h, mask){

        e.postMessage(1);
        var raw = sock.recv(acceptSock, 1024);
        parser.reinitialize(httpParser.REQUEST);
        parser.parse(raw);

        //console.log(parser);
        
        var n = sock.send(acceptSock, html, html.length, 0);
        if (n === null){
            throw new Error("can't send to host " + process.errno);
        }

        if (!sock.close(acceptSock)){
            //ignore close error
        }
        
        h.io_close();
    });
    return 1;
}

/* avoid calling this from workers, just initiate from main thread */
if (process.main) {
    main();
}
