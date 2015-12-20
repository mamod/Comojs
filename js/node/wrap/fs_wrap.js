var posix = process.binding('posix');
var sys   = process.binding('sys');
var isWin = process.platform == 'win32';
var util  = require('util');

var fs = process.binding('fs');
var errnoException = require('util')._errnoException;

var statFunction;

function _throw (errno, syscall, path){
    var error = errnoException(errno, syscall);
    if (path) error.path = path.replace(/^\\\\\?\\/, '');
    return error;
}

exports.FSReqWrap = function(){};

exports.FSInitialize = function(fn){
    statFunction = fn;
};

exports.open = function(file, flags, mode, req){
    var err = null;
    var fd = posix.open(file, flags, mode);
    if (fd === null){
        err = _throw(process.errno, 'open', file);
    } else {
        var closefd = isWin ? sys.GetOsFHandle(fd) : fd;
        if (!sys.cloexec(closefd, 1)){
            err = _throw(process.errno, 'open', file);
        }
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err, fd);
        });
        return;
    }

    if (err) throw(err);
    return fd;
};

exports.close = function(fd, req){
    var err;
    var fd = posix.close(fd);
    if (fd === null){
        err = _throw(process.errno, 'close');
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.read = function(fd, buffer, offset, length, position, req){
    offset = offset || 0;
    length = length || buffer.byteLength;

    var err;
    var nread = posix.read(fd, [buffer, offset], length, position);
    if (nread === null){
        err = _throw(process.errno, 'read');
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err, nread);
        });
        return;
    }

    if (err) throw(err);
    return nread;
};

exports.writeBuffers = function(fd, chunks, pos, req){
    if (req){
        cb = {};
        cb.oncomplete = function(err, n){
            if (err){
                req.oncomplete(err);
                return;
            }

            var chunk = chunks.shift();
            if (!chunk){
                req.oncomplete(err, n);
                return;
            }

            exports.writeBuffer(fd, chunk, 0, chunk.byteLength, pos, cb);
        };

        cb.oncomplete(null);
        return;
    }

    chunks.forEach(function(chunk){
        exports.writeBuffer(fd, chunk, 0, chunk.byteLength, pos);
    });
};

exports.writeBuffer = exports.writeString = function(fd, data, offset, length, position, req){
    var err;
    var buffer;

    if (util.isBuffer(data)){
        buffer = data;
        if (!util.isNumber(position)){
            position = 0;
        }
    } else {
        var args = [].slice.call(arguments);
        position = args[2];
        if (!util.isNumber(position)){
            position = 0;
        }

        var encoding = args[3];
        req = args[4];

        if (!util.isString(encoding)){
            encoding = 'utf8';
            req = encoding;
        }

        buffer = Buffer(data, encoding);
        length = buffer.byteLength;
        offset = 0;
    }

    var nwritten = posix.writeBuffer(fd, [buffer, offset], length, position);
    if (nwritten === null){
        err = _throw(process.errno, 'write');
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err, nwritten);
        });
        return;
    }

    if (err) throw(err);
    return nwritten;
};

function _normalizePath (args){
    if (typeof args[0] === 'string'){
    args[0] = args[0].replace(/^\\\\\?\\/, '');
    }
    return args;
}

function _normalizeLinkArgs (args) {
    //src => args[0]
    if (!util.isString(args[0])){
        throw new Error('src path must be a string');
    }

    //dest => args[1]
    if (!util.isString(args[1])){
        throw new Error('dest path must be a string');
    }

    args[0] = _normalizePath(args[0]);
    args[1] = _normalizePath(args[1]);

    return args;
}

function _checkAccessArgs (args){
     if (!util.isString(args[0])){
        throw new Error("path must be a string");
    }
    return args;
}

[
    ['close', 2],
    ['ftruncate', 3],
    ['rmdir', 3],
    ['mkdir', 3],
    ['fchmod', 3],
    ['chmod', 3],
    ['unlink', 2],
    ['symlink', 4],
    ['readdir', 2],
    ['fsync', 2],
    ['link', 3, _normalizeLinkArgs],
    ['access', 3, _checkAccessArgs]
].forEach(function(obj){
    var fn         = obj[0];
    var argsLength = obj[1];
    var _normalize = obj[2] || _normalizePath;

    exports[fn] = function(){
        var args = [].slice.call(arguments);
        var req;
        var err = null;

        if (args.length === argsLength){
            req = args.pop();
        }

        args = _normalize(args);
        // args[0] = _normalizePath(args[0]);

        var ret = posix[fn].apply(null, args);

        if (ret === null){
            err = _throw(process.errno, fn, args[0]);
        }

        if (req){
            process.nextTick(function(){
                req.oncomplete(err, ret);
            });
            return;
        }

        if (err) throw err;
        return ret;
    };
});

exports.fdatasync = exports.fsync;


['lstat', 'fstat', 'stat'].forEach(function(fn){
    exports[fn] = function(file, req){
        var err = null;
        var stat;

        if (typeof file === 'string'){
            file = file.replace(/^\\\\\?\\/, '');
        }

        var s = posix[fn](file);
        if (s === null){
            err = _throw(process.errno, fn, file);
        }

        if (s && !err){
            stat = new statFunction(s.dev,
                s.mode,
                s.nlink,
                s.uid,
                s.gid,
                s.rdev,
                s.blksize,
                s.ino,
                s.size,
                s.blocks,
                s.atime * 1000, //to millisecond
                s.mtime * 1000, //to millisecond
                s.ctime * 1000, //to millisecond
                s.birthtim_msec
            );
        }

        if (req){
            process.nextTick(function(){
                req.oncomplete(err, stat);
            });
            return;
        }

        if (err) throw err;
        return stat;
    };
});
