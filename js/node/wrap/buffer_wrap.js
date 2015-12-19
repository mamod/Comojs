var binding = process.binding('buffer');

function _enc (enc){
    enc = typeof enc === 'string' ? enc.toLowerCase() : 'utf8';
    var enc_num = binding.encodings[enc];
    if (!enc_num) throw new Error('Unknown encoding: ' + enc);
    return enc_num;
}

exports.setupBufferJS = function(proto, obj){
    proto.hexSlice = binding.hexSlice;
    proto.utf8Write = binding.utf8Write;
    proto.binaryWrite = binding.binaryWrite;

    obj.flags = new Uint8Array(1);
};

exports.createFromString = function(string, encoding){
    var enc = _enc(encoding);
    if (enc === 2) return new DUK_Buffer(string);

    return new DUK_Buffer(binding.create(string, enc));
};

exports.fill = binding.fill;

exports.byteLengthUtf8 = binding.byteLengthUtf8;
exports.create = binding.create;
exports.kMaxLength = 1073741823;
