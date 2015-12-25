var binding = process.binding('buffer');
var _bufferPrototype;

function _enc (enc){
    return binding.encodings[enc] || 3;
}

exports.setupBufferJS = function(proto, obj){
    _bufferPrototype = proto;

    proto.hexSlice     = binding.hexSlice;
    proto.asciiSlice   = binding.asciiSlice;
    proto.base64Slice  = binding.base64Slice;
    proto.binarySlice  = binding.asciiSlice;
    proto.utf8Slice  = binding.utf8Slice;
    proto.utf8Slice    = function(s,e){
        return NODE_BUFFER.prototype.toString.call(this, 'utf8', s, e);
    };

    proto.hexWrite = binding.hexWrite;
    proto.utf8Write = binding.utf8Write;
    proto.asciiWrite = binding.asciiWrite;
    proto.binaryWrite = binding.binaryWrite;
    proto.base64Write = binding.base64Write;

    proto.copy = NODE_BUFFER.prototype.copy;

    obj.flags = new Uint8Array(1);
};

exports.createFromString = function(string, encoding){
    var enc = _enc(encoding);
    var buf = binding.createFromString(string, enc);
    Object.setPrototypeOf(buf, _bufferPrototype);
    return buf;
};

exports.createFromArrayBuffer = function(ab){
    if(!(ab instanceof ArrayBuffer)) throw("argument is not an ArrayBuffer");
    var buf = new Uint8Array(ab, 0, ab.byteLength);
    Object.setPrototypeOf(buf, _bufferPrototype);
    return buf;
};

exports.fill    = binding.fill;
exports.compare = binding.compare;

exports.byteLengthUtf8 = binding.byteLengthUtf8;
exports.create = binding.create;
exports.kMaxLength = binding.kMaxLength;
