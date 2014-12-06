var Worker  = require("worker");
var sock    = process.binding('socket');
var binding = process.binding('io');
var loop    = require('loop').default_loop;

var httpParser = require('http_parser');
var assert = require('assert');

var parser = new httpParser(httpParser.RESPONSE);
var html =  "HTTP/1.1 200 OK\r\n" +
                "Content-Type: text/html; charset=utf-8\r\n" +
                "Content-Length: 8\r\n\r\n" +
                "Hi There";

var expected_headers = ['Host',
    'http://127.0.0.1:9090',
    'Connection',
    'close',
    'x-test',
    'hi' ];

function main (){

    //simple server here
    var s = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
    var ip = sock.pton('127.0.0.1', 9090);
    
    if (!sock.setsockopt(s, sock.SOL_SOCKET,
                          sock.SO_REUSEADDR, 1)){
        
        throw new Error("error " . process.errno);
    }

    if (!sock.bind(s, ip)){
        throw new Error(" Error Binding " + process.errno);
    }

    

    sock.nonblock(s, 1);

    if (!sock.listen(s, sock.SOMAXCONN)){
        throw new Error("Error listening " + process.errno);
    }

    var i = 0;
    
    function sockcb (handle, mask) {

        var acceptSock = sock.accept(s);
        if (!acceptSock) {
            throw new Error("can't accept");
        }

        var cl = loop.io_start(acceptSock, loop.POLLIN, function(h, mask){

            var raw = sock.recv(acceptSock, 1024);
            if (raw === null){
                throw new Error("Error Recieving from host " + process.errno);
            }

            parser.reinitialize(httpParser.REQUEST);
            parser.parse(raw);

            //every response must has the exact headers
            assert.deepEqual(parser._headers, expected_headers);

            if (!sock.send(acceptSock, html, html.length, 0)){
                throw new Error(process.errno);
            }

            if (!sock.close(acceptSock)){
                //ignore close errors
            }

            h.io_close();
        });

        i++;
        assert(i <= 1000);
        if (i == 1000){
            handle.io_close();
            //set some delay to make sure that no worker will
            //post another message later
            setTimeout(function(){}, 100);
        }
    }

    var handle = loop.io_start(s, loop.POLLIN, sockcb);
    console.log("listening on port 9090");

    var w = new Worker(__filename, 4);

    w.onerror = function(e) {
        assert.fail(e.data);
    }

    for (var x = 0; x < 100; x++){
        w.postMessage(1);
    }

    process.on("exit", function(){
        assert.equal(i, 1000);
    });
}

exports.onmessage = function(e){
    var http = require('../../lib/http.js');

    try {
        for (var i = 0; i < 10; i++){
            var t1 = http.Request({
                host   : 'http://127.0.0.1',
                path   : '/test',
                method : 'GET',
                port : 9090,
                headers : {
                    'x-test' : 'hi'
                },
                body : ["Hi", "there"]
            });
        }
    } catch(e){
        process.reportError(e);
    }
};

if (process.main){
    main();
}
