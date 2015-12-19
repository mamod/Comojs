var errno = process.binding('errno');
var ERROR_MAP = {};

Object.keys(errno).forEach(function(key) {
    var val = errno[key];
    ERROR_MAP[val] = key;
});

exports.UV_EINVAL = errno.EINVAL;
exports.UV_EOF = -4095;

exports.errname = function(err){
    if (err < 0) err = -err;
    return ERROR_MAP[err];
};
