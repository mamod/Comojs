
var sock    = process.binding('socket');
var io      = require('io');
var uv      = require('uv');
exports.TCP = TCP;
exports.TCPConnectWrap = TCPConnectWrap;

function TCPConnectWrap (){}

function TCP (){
    this.writeQueueSize = 0;
    this._handle = new uv.TCP();
}

TCP.prototype.bind6 = TCP.prototype.bind = function(ip, port){
    var addr = sock.pton(ip, port);
    if (!addr){
        return process.errno;
    }

    var err = this._handle.bind(addr, 0);
    return err;
};

TCP.prototype.close = function(cb){
    if (this.reading || this.owner) {
        this._handle.close(cb);
    } else {
        //TODO: need to free resources here
        sock.close(this._handle.fd);
    }
};

TCP.prototype.listen = function(backlog){
    var tcp = this;
    //pass onConnection callback
    return this._handle.listen(backlog, function(status){
        var client;
        if (status === 0){
            client = new TCP();
            this.accept(client._handle);
        }
        tcp.onconnection(status, client);
    });
};

TCP.prototype.readStart = function(){
    var tcp = this;
    this._handle.read_start(function(err, buf){
        var len;
        if (err){
            len = err;
        } else if (buf){
            len = buf.length;
        } else { len = 0; }

        tcp.onread(len, buf);
    });
};

TCP.prototype.readStop = function(){
    return this._handle.read_stop();
};

TCP.prototype.shutdown = function(req){
    var tcp = this;
    this._handle.shutdown(function(){
        req.oncomplete(0, tcp, req);
    });
};

TCP.prototype.writeBuffer = TCP.prototype.writeUtf8String = function(req, data){
    this._handle.write(data);
    return 0;
};

TCP.prototype.connect = function(req_wrap_obj, ip_address, port){
    var tcp = this;
    var addr = uv.ip4_addr("127.0.0.1", port);
    if (addr === null){
        return process.errno;
    }

    // var req_wrap = new TCPConnectWrap(req_wrap_obj);
    var err = this._handle.connect(addr, function AfterConnect (status){
        req_wrap_obj.oncomplete(status, tcp, req_wrap_obj, true, true);
    });
    return err;
};
