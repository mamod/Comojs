
var util         = require('util');
var loop         = require('loop');
var HTTPparser   = require('http_parser');
var handle       = require('handle');
var events       = require('events');
var assert       = require('assert');
var socket       = process.binding('socket');
var errno        = process.binding('errno');
var io           = process.binding('io'); //for select

var CRLF         = '\r\n';
var default_loop = loop.default_loop;

var customOptions = {
    port   : 80,
    path   : '/',
    method : 'GET',
    localPort : 0,
    localAddress : '0.0.0.0',
    timeout : 5000,
    autodie : false,
    body  : null
};

util.inherits(Request, events.EventEmitter);
function Request (options, cb){
    if (!(this instanceof Request)) {
        return new Request(options, cb);
    }

    var self = this;

    //handle and extend options
    handle.Options(options, customOptions);
    self.options = options;

    self.async = (cb && typeof cb === 'function');
    if (self.async){
        self.cb = cb;
    }

    if (!self.options.host){
        throw new Error('options missing a request host');
    }

    var proto = socket.getprotobyname('tcp');
    if (!proto){
        throw new Error('Error proto');
    }

    //construct local ip address
    self.ip = socket.pton(options.localAddress, options.localPort);
    if (!self.ip){
        throw new Error('bad ip address');
    }

    //create ip pointer for host
    self.hostip = socket.pton('127.0.0.1', options.port);
    if (!self.hostip){
        throw new Error("invalid host");
    }

    while (1){

        //create local socket for connection
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, proto);
        if (!self.sock){
            throw new Error("Couldn't Create new socket");
        }

        //FIXME .. Should we set reuse option
        if (!socket.setsockopt(self.sock, socket.SOL_SOCKET,
                      socket.SO_REUSEADDR, 1)){
            throw new Error("error " . process.errno);
        }
        
        if (!socket.nonblock(self.sock, 0)){
            throw new Error('sock block');
        }

        //FIXME ... Should we bind here
        if (!socket.bind(self.sock, self.ip)){
            throw new Error("Error while binding");
        }

        process.errno = 0;
        var conn;
        do {
            conn = socket.connect(self.sock, self.hostip);
        } while (!conn && process.errno === errno.EINTR);

        if (process.errno){
            
            //on error connecton we need to close and reoprn a new socket
            //connect(2) man page
            //If connect() fails, consider the state of the socket as unspecified.
            //Portable applications should close the socket and create a new one
            //for reconnecting.

            //ignore errors
            socket.close(self.sock);
            
            if (process.errno === errno.EADDRINUSE){
                process.sleep(1);
                continue;
            }

            //connection refused
            if (process.errno === errno.ECONNREFUSED){
                throw new Error("Connection Refused");
            }
            
            throw(process.errno);
        } else {
            break;
        }
    }

    try {
        self.prepareHeaders();
        self.startReading();
    } catch (e){
        socket.close(self.sock);
        throw(e);
    }

    return this;
}

Request.prototype._setHeader = function (name, val) {
    this.headers += name + ': ' + val + CRLF;
};

Request.prototype.prepareHeaders = function (){
    var self = this;

    var options = self.options;
    var headers = options.headers;

    var firstLine = options.method + ' ' + options.path + 
                  ' HTTP/1.1' + CRLF;

    self.headers = firstLine;

    if (options.port !== 80){
        self._setHeader("Host", options.host + ':' + options.port);
    } else {
        self._setHeader("Host", options.host);
    }

    if (options.method.toUpperCase() === 'POST'){
        self._setHeader("Content-Type", "application/x-www-form-urlencoded");
    }
    //self._setHeader("Transfer-Encoding", "chunked");
    self._setHeader("Connection", "close");

    //send optional headers
    if (headers){
        for (var key in headers){
            self._setHeader(key, headers[key]);
        }
    }
    
    //end of headers
    self.headers += CRLF;
    while (1){
        self.can_write();
        var _length = self.headers.length;
        var n = socket.send(self.sock, self.headers, _length, 0);
        if (n === null){
            throw new Error("Error while sending to host " + process.errno);
        } else if (n !== _length){
            console.log(self.headers);
            self.headers = self.headers.substr(n);
            process.sleep(1);
            continue;
        } else {
            break;
        }
    }
};

var ttt = 0;
var on_headers_complete = function(val){
    
};

var on_body = function(val){
    this.emit("data", val);
};

var _Read_and_Parse = function (self) {
    var data = socket.recv(self.sock, 1024);
    if (data === null){
        throw new Error("connection closed " + process.errno);
    }
    //eof
    else if (String(data).length === 0){
        throw('EOF');
    }

    var n = self.parser.parse(data);
};

Request.prototype.can_write = function (){
    var self = this;
    var ret = io.can_write(self.sock, self.options.timeout);
    if (ret === null){
        throw new Error("Can't Write to Host");
    } else if (ret === 0){
        throw new Error("Writing to host timedout");
    }
};

Request.prototype.can_read = function (){
    var self = this;
    self.start = self.start || Date.now();
    var ret = io.can_read(self.sock, self.options.timeout);
    if (!ret){
        //issue a graceful close
        //socket.shutdown(self.sock, socket.SHUT_BOTH);
        if (ret === null){
            throw new Error("Can't Read From Host");
        } else if (ret === 0){
            throw new Error("Reading From Host Timedout");
        }
    }
};

Request.prototype.startReading = function (){
    var self = this;

    //create a new parser
    self.parser = new HTTPparser(HTTPparser.REQUEST);
    self.parser.on_headers_complete = on_headers_complete;
    self.parser.on_body = on_body.bind(self);
    
    var keepReading = 1; //reading signal
    self.parser.on_message_complete = function(){
        //signal stopreading
        keepReading = 0;
        //close socket
        socket.shutdown(self.sock, 2);
        if (!socket.close(self.sock)){
            throw new Error('can not close');
        }
    };

    //blocking request
    while (keepReading){
        self.can_read();
        _Read_and_Parse(self);
        if (keepReading) process.sleep(1);
    }
};

module.exports = Request;
