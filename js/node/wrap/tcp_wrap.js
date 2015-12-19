var sock    = process.binding('socket');
var uv      = require('uv');
exports.TCP = TCP;
exports.TCPConnectWrap = TCPConnectWrap;

var MakeCallback = process.MakeCallback;

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
    var tcp = this;
    process.nextTick(function(){
        tcp._handle.close(cb);
    });
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
        MakeCallback(tcp, "onconnection", status, client);
        // tcp.onconnection(status, client);
    });
};

TCP.prototype.readStart = function(){
    var tcp = this;
    this._handle.read_start(function(err, buf){
        var len;
        if (err){
            len = err > 0 ? -err : err;
        } else if (buf){
            len = buf.length;
        } else { len = 0; }

        MakeCallback(tcp, "onread", len, buf);
        // tcp.onread(len, buf);
    });
};

TCP.prototype.readStop = function(){
    return this._handle.read_stop();
};

TCP.prototype.shutdown = function(req){
    var tcp = this;
    this._handle.shutdown(function(status){
        MakeCallback(req, "oncomplete", status, tcp, req);
        // req.oncomplete(0, tcp, req);
    });
};

TCP.prototype.writeBinaryString = function(req, data){
    data = Buffer(data, "binary");
    return this.writeUtf8String(req, data.toString("binary"));
};

TCP.prototype.writeBuffer = TCP.prototype.writeUtf8String = function(req, data){
    var tcp = this;
    this._handle.write(data, function(status){
        tcp.writeQueueSize = tcp._handle.write_queue_size;
        req.bytes = this.bytes;
        MakeCallback(req, "oncomplete", status, tcp, req, 0);
    });
    
    return 0;
};

TCP.prototype.connect = function(req_wrap_obj, ip_address, port){
    var tcp = this;
    var addr = uv.ip4_addr("127.0.0.1", port);
    if (addr === null){
        return process.errno;
    }
    
    var err = this._handle.connect(addr, function AfterConnect (status){
        MakeCallback(req_wrap_obj, "oncomplete", status, tcp, req_wrap_obj, true, true);
        //req_wrap_obj.oncomplete(status, tcp, req_wrap_obj, true, true);
    });
    return err;
};
