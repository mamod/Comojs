var bindings = process.binding('tty');
var uv       = require('uv');

exports.guessHandleType = bindings.guessHandleType;

function TTY (fd, readable){
    this._handle = new uv.TTY(fd, readable);
    this.readStart(function(){});
};

TTY.prototype.writeUtf8String = function(req, data){
    this._handle.write(data);
};

TTY.prototype.getWindowSize = function(arr){
    var winsize = bindings.getWindowSize();
    if (!winsize){
        return process.errno;
    }

    arr[0] = winsize.width;
    arr[1] = winsize.height;
    return 0;
};

TTY.prototype.readStart = function(){
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

TTY.prototype.readStop = function(){
    return this._handle.read_stop();
};

TTY.prototype.setRawMode = function(mode){
    this._handle.set_mode(mode ? 1 : 0);
};

exports.TTY = TTY;
