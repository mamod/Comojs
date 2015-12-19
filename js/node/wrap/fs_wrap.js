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
    var err;
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

exports.stat = function(file, req){

    var err = null;
    var stat;

    file = file.replace(/^\\\\\?\\/, '');

    var s = posix.stat(file);
    if (s === null){
        err = _throw(process.errno, 'stat', file);
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

    if (err) throw(err);
    return stat;
};

exports.fstat = function(fd, req){
    var err;
    var stat;
    var s = posix.fstat(fd);
    if (s === null){
        err = _throw(process.errno, 'stat');
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

    if (err) throw(err);
    return stat;
};

exports.lstat = function(file, req){
    file = file.replace(/^\\\\\?\\/, '');
    var err;
    var stat;
    var s = posix.lstat(file);
    if (s === null){
        err = _throw(process.errno, 'stat');
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

    if (err) throw(err);
    return stat;
};

exports.read = function(fd, buffer, offset, length, position, req){

    offset = offset || 0;
    length = length || buffer.byteLength;
    // position = position || 0;
// console.log(arguments);
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

exports.unlink = function(path, req){
    var err = null;
    if (!posix.unlink(path)){
        err = _throw(process.errno, 'unlink', path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.access = function(path, mode, req){
    var err = null;

    if (!util.isString(path)){
        throw new Error("path must be a string");
    }

    if (!posix.access(path, mode)){
        err = _throw(process.errno, 'unlink', path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.chmod = function(path, mode, req){
    var err = null;
    if (!posix.chmod(path, mode)){
        err = _throw(process.errno, 'chmod', path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.fchmod = function(fd, mode, req){
    var err = null;
    if (!posix.fchmod(fd, mode)){
        err = _throw(process.errno, 'fchmod');
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.mkdir = function(path, mode, req){
    var err = null;
    if (!posix.mkdir(path, mode)){
        err = _throw(process.errno, 'mkdir ' + path, path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.rmdir = function(path, mode, req){
    var err = null;
    if (!posix.rmdir(path, mode)){
        err = _throw(process.errno, 'rmdir', path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.fdatasync = exports.fsync = function(fd, req){
    var err = null;
    if (!posix.fsync(fd)){
        err = _throw(process.errno, 'fsync');
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err);
        });
        return;
    }

    if (err) throw(err);
};

exports.readdir = function(path, req){
    path = path.replace(/^\\\\\?\\/, '');
    var err = null;
    var readPath = isWin ? path + '\\*' : path;
    var result = posix.readdir(readPath);
    if (!result){
        err = _throw(process.errno, 'readdir', path);
    }

    if (req){
        process.nextTick(function(){
            req.oncomplete(err, result);
        });
        return;
    }

    if (err) throw(err);
    return result;
};
