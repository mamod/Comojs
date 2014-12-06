var util         = require('util');
var loop         = require('loop').default_loop;
var LOOP         = require('loop');
var HTTPparser   = require('http_parser');
var handle       = require('handle');
var events       = require('events');
var stream       = require('stream');

var sock         = process.binding('socket');
var errno        = process.binding('errno');
var io           = process.binding('io'); //for select

var isWin        = process.platform === 'win32';
var CRLF         = '\r\n';

var customOptions = {
    port     : 9090, //will assign to random port
    address  : '0.0.0.0',
    timeout  : 5000, //default timeout
    autodie  : false,
    writable : true,
    readable : true,
    async    : true
};

function Server (options, cb){
    if (!(this instanceof Server)) {
        return new Server(options, cb);
    }

    var self = this;
    self._connections = 0;

    //handle and extend options
    self.options = handle.Options(options, customOptions);
    self.stream  = stream.Duplex.call({}, self.options);

    return self;
}

Server.prototype.on = function(a,b,c){
    this.stream.on(a,b,c);
};

//util.inherits(Socket, stream.Duplex);
function Socket (fd){
    var self = this;
    var accept = sock.accept(fd);
    if (!accept) {
        throw new Error("can't accept");
    }
    
    if (!sock.nonblock(accept, 1)){
        throw new Error("Error Setting Socket to nonblocking");
    }
    //stream.Duplex.call(this, {});
    self.fd = accept;
    self.handle = loop.init_handle(function(h, mask){
        if (mask & LOOP.POLLIN){
            var d = sock.recvBuffer(accept, 1024, 0);
            if (d === null){
                //check for EWOULDBLOCK & EINTER

                throw new Error(process.errno);
            }

            //EOF
            if (d.length === 0){
                //stop pollin/read watcher
                h.io_stop(LOOP.POLLIN);

                //start writing watcher
                h.io_start(accept, LOOP.POLLOUT);
            } else {
                print(d.toString());
                //sock.send(accept, d.toString(), d.length, 0);
            }
        } else {
            h.io_close();
            sock.close(accept);
        }
    });

    self.handle.io_start(accept, LOOP.POLLIN | LOOP.POLLERR);
};

Socket.prototype._read = function(){

};

Server.prototype.listen = function(port){
    var self = this;
    if (!port) port = self.options.port;

    //create new tcp
    var fd = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
    if (!fd){
        throw new Error('can not create new socket');
    }

    //non blocking
    if (!sock.setsockopt(fd, sock.SOL_SOCKET,
                      sock.SO_REUSEADDR, 1)){
    
        throw new Error("error " . process.errno);
    }

    var localIP = sock.pton('127.0.0.1', 9090);
    if (!localIP){
        throw new Error('can not create new ip');
    }

    if (!sock.bind(fd, localIP)){
        throw new Error('can not bind');
    }

    self.fd = fd;
    self.localIP = localIP;
    
    if (!sock.nonblock(fd, 1)){
        throw new Error("can't set fd to nonblocking");
    }

    if (!sock.listen(fd, sock.SOMAXCONN)){
        throw new Error("can't listen to " + fd);
    }

    var onclose = function(){
        //clear
        sock.close(fd);
        //free ip address

        throw new Error('close');
    };

    self.on("close", onclose);

    this.handle = loop.init_handle(function(){
        self._connections++;
        var socket = new Socket(self.fd);
        self.stream.emit("connection", socket);
    });

    this.handle.io_start(fd, LOOP.POLLIN);

    return this;
};

module.exports = Server;
