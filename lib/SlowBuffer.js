var NativeBuffer = process.binding('buffer');

function createSlice (type) {
    return function Slice (start,end){
        return NativeBuffer.slice(this.pointer,type,start,end);
    };
}

function createWrite (type) {
    return function Write (string, offset, max) {
        return NativeBuffer.write(this.pointer, type, string, offset, max);
    };
}

function SlowBuffer(length) {
    this.pointer = NativeBuffer.SlowBuffer(length);
    this.length = length;
}

SlowBuffer.prototype.set = function (offset, ch){
    NativeBuffer.set(this.pointer, offset, ch);
};

SlowBuffer.prototype.get = function (offset){
    return NativeBuffer.get(this.pointer, offset);
};

SlowBuffer.prototype.hexWrite    = createWrite(NativeBuffer.hex);
SlowBuffer.prototype.base64Write = createWrite(NativeBuffer.base64);
SlowBuffer.prototype.utf8Write   = createWrite(NativeBuffer.utf8);
SlowBuffer.prototype.binaryWrite = createWrite(NativeBuffer.binary);
SlowBuffer.prototype.asciiWrite  = createWrite(NativeBuffer.ascii);
SlowBuffer.prototype.ucs2Write   = createWrite(NativeBuffer.ucs2);

SlowBuffer.prototype.hexSlice    = createSlice(NativeBuffer.hex);
SlowBuffer.prototype.utf8Slice   = createSlice(NativeBuffer.utf8);
SlowBuffer.prototype.binarySlice = createSlice(NativeBuffer.binary);
SlowBuffer.prototype.base64Slice = createSlice(NativeBuffer.base64);

SlowBuffer.prototype.fill = function (val,start,end) {
    return NativeBuffer.fill(this.pointer, val, start, end);
};

SlowBuffer.prototype.copy = function (target, target_start, start, end) {
    return NativeBuffer.copy(this.pointer, target.pointer, target_start,
                             start, end);
};

SlowBuffer.byteLength = function (str, enc){
    var ret;
    str = str + '';
    switch (enc) {
        case 'ascii':
        case 'binary':
        case 'raw':
            ret = str.length;
            break;
        case 'ucs2':
        case 'ucs-2':
        case 'utf16le':
        case 'utf-16le':
            ret = str.length * 2;
            break;
        case 'hex':
            ret = str.length >>> 1;
            break;
        default:
            ret = NativeBuffer.byteLength(str, NativeBuffer[enc]);
    }
    return ret;
};

Duktape.fin(SlowBuffer.prototype, function (buf) {
    if (buf === SlowBuffer.prototype) {
        return;
    }
    if (buf.pointer === null) {
        return; //already freed
    }
    try {
        NativeBuffer.destroy(buf.pointer);
    } catch (e) {}
    buf.pointer = null;
});

exports.SlowBuffer = SlowBuffer;
