
"use strict";

var assert  = require('assert').ok;
var path    = require('path');
var util    = require('util');
var binding = process.binding('io');
var error   = require('error');

var O_APPEND = binding.O_APPEND || 0;
var O_CREAT  = binding.O_CREAT  || 0;
var O_EXCL   = binding.O_EXCL   || 0;
var O_RDONLY = binding.O_RDONLY || 0;
var O_RDWR   = binding.O_RDWR   || 0;
var O_SYNC   = binding.O_SYNC   || 0;
var O_TRUNC  = binding.O_TRUNC  || 0;
var O_WRONLY = binding.O_WRONLY || 0;

module.exports = {
    cloexec : cloexec,
    nonblock : nonblock,
    open     : open
};

function open (file, flags, mode) {
    return new IOFile(file, flags, mode);
}

function cloexec (fd, set) {
    return 1;
}

function nonblock (fd, set) {
    return 1;
}

function stringToFlags(flag) {
    // Only mess with strings
    if (!util.isString(flag)) {
        return flag;
    }
    switch (flag) {
        case 'r' : return O_RDONLY;
        case 'rs' : // fall through
        case 'sr' : return O_RDONLY | O_SYNC;
        case 'r+' : return O_RDWR;
        case 'rs+' : // fall through
        case 'sr+' : return O_RDWR | O_SYNC;
        case 'w' : return O_TRUNC | O_CREAT | O_WRONLY;
        case 'wx' : // fall through
        case 'xw' : return O_TRUNC | O_CREAT | O_WRONLY | O_EXCL;
        case 'w+' : return O_TRUNC | O_CREAT | O_RDWR;
        case 'wx+': // fall through
        case 'xw+': return O_TRUNC | O_CREAT | O_RDWR | O_EXCL;
        case 'a' : return O_APPEND | O_CREAT | O_WRONLY;
        case 'ax' : // fall through
        case 'xa' : return O_APPEND | O_CREAT | O_WRONLY | O_EXCL;
        case 'a+' : return O_APPEND | O_CREAT | O_RDWR;
        case 'ax+': // fall through
        case 'xa+': return O_APPEND | O_CREAT | O_RDWR | O_EXCL;
    }
    throw new Error('Unknown file open flag: ' + flag);
}

//IOFile
function IOFile (file, flags, mode) {
    flags = flags || 'r';
    mode = mode || 438;

    this.file  = path.resolve(file);
    this.flags = stringToFlags(flags);
    this.mode  = mode;
    var fd = binding.open(this.file, this.flags, this.mode);
    if (fd === null) {
        throw new Error(999);
    }
    this.fd = fd;
    return this;
}

IOFile.prototype.stat = function (){
    return this.fd;
};

IOFile.prototype.read = function (n){
    var fd = this.fd;
    if (fd === null) {
        throw new Error('fd alreday closed');
    }
    
    return binding.read(fd, n);
    return buff;
};
