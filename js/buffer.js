
var binding = process.binding('buffer');
var util    = require('util');
var assert  = require('assert');

var pool = 16 * 1024;
var allocBuffer = Duktape.Buffer(pool);
Buffer.INSPECT_MAX_BYTES = 50;

function _enc (enc){
    enc = typeof enc === 'string' ? enc.toLowerCase() : 'utf8';
    var enc_num = binding.encodings[enc];
    if (!enc_num) throw new Error('Unknown encoding: ' + enc);
    return enc_num;
}

function Buffer (buf, encoding){
    if (this instanceof Buffer){
        throw new Error("Buffer is ot a constructor");
    }

    if (util.isNumber(buf)){
        if (buf < 0) buf = 0;
        return Duktape.Buffer(buf);
    } else if (util.isString(buf)) {
        if (encoding && encoding !== 'utf8'){
            return binding.create(buf, _enc(encoding));
        }
        return Duktape.Buffer(buf);
    } else if (util.isNumber(buf.length) || util.isArray(buf)){
        var buf2 = Duktape.Buffer(buf.length);
        for (var i = 0; i < buf.length; i++){
            buf2[i] = buf[i];
        }
        return buf2;
    } else if (typeof buf === 'object'){
        if (buf.type && buf.type === 'Buffer'){
            return Buffer(buf.data);
        }
        return Duktape.Buffer(0);
    } else {
        return buf;
    }
}

Duktape.Buffer.prototype.inspect = function inspect() {
    var str = '';
    var max = Buffer.INSPECT_MAX_BYTES;
    if (this.length > 0) {
        str = this.toString('hex', 0, max).match(/.{2}/g).join(' ');
        if (this.length > max)
            str += ' ... ';
    }
    return '<' + this.constructor.name + ' ' + str + '>';
};

Duktape.Buffer.prototype.toString = function toString(encoding, start, end){
    var length = this.length;
    
    start = start >>> 0;
    end = util.isUndefined(end) || end === Infinity ? length : end >>> 0;

    if (!encoding) encoding = 'utf8';
    if (start < 0) start = 0;
    if (end > length) end = length;
    if (end <= start) return '';

    assert(start < end);
    assert(end <= length);

    return binding.slice(this.valueOf(), _enc(encoding), start, end);
};

Duktape.Buffer.prototype.toJSON = function() {
    return {
        type: 'Buffer',
        data: Array.prototype.slice.call(this, 0)
    };
};

//write(string[, offset[, length]][, encoding])
Duktape.Buffer.prototype.write = function (string, offset, length, encoding){
    // Buffer#write(string);
    if (util.isUndefined(offset)) {
        length = this.length;
        offset = 0;
        // Buffer#write(string, encoding)
    } else if (util.isUndefined(length) && util.isString(offset)) {
        encoding = offset;
        length = this.length;
        offset = 0;
        // Buffer#write(string, offset[, length][, encoding])
    } else if (isFinite(offset)) {
        offset = offset >>> 0;
        if (isFinite(length)) {
            length = length >>> 0;
        } else {
            encoding = length;
            length = undefined;
        }
    }

    encoding = _enc(encoding);
    var remaining = this.length - offset;
    if (util.isUndefined(length) || length > remaining){
        length = remaining;
    }

    if (string.length > 0 && (length < 0 || offset < 0)) {
        throw new RangeError('attempt to write outside buffer bounds');
    }

    if (string.length == 0 && offset > 0){
        throw new RangeError('attempt to write outside buffer bounds');
    }

    return binding.write(this.valueOf(), string, encoding, offset, length);
};

Duktape.Buffer.prototype.fill = function fill(value, start, end) {
    value || (value = 0);
    start || (start = 0);
    end || (end = this.length);

    if (end < start) throw new RangeError('end < start');

    // Fill 0 bytes; we're done
    if (end === start) return 0;
    if (this.length == 0) return 0;

    if (start < 0 || start >= this.length) {
        throw new RangeError('start out of bounds');
    }

    if (end < 0 || end > this.length) {
        throw new RangeError('end out of bounds');
    }

    binding.fill(this.valueOf(), value, start, end);
    return this;
};

// copy(targetBuffer, targetStart=0, sourceStart=0, sourceEnd=buffer.length)
Duktape.Buffer.prototype.copy = function(target, targetStart, sourceStart, sourceEnd) {
    
    var target_length = target.length;
    var source_length = this.length;

    if (typeof targetStart === 'undefined'){
        targetStart = 0;
        sourceStart = 0;
        sourceEnd = source_length;
    } else if (typeof sourceStart === 'undefined'){
        sourceStart = 0;
        sourceEnd  = source_length;
    } else if (typeof sourceEnd === 'undefined'){
        sourceEnd = source_length;
    }
    
    if (sourceEnd < 0 || sourceStart < 0){
        throw new RangeError("out of range index");
    }

    if (targetStart >= target_length || sourceStart >= sourceEnd) {
        return Buffer(0);
    }

    if (target_length - targetStart < sourceEnd - sourceStart){
        sourceEnd = target_length - targetStart + sourceStart;
    }

    return binding.copy(this.valueOf(),
                        target.valueOf(),
                        targetStart,
                        sourceStart,
                        sourceEnd);
};

Duktape.Buffer.prototype.slice = function(start, end) {
    var len = this.length;
    start = ~~start;
    end = util.isUndefined(end) ? len : ~~end;

    if (start < 0) {
        start += len;
        if (start < 0) start = 0;
    } else if (start > len) {
        start = len;
    }

    if (end < 0) {
        end += len;
        if (end < 0) end = 0;
    } else if (end > len) {
        end = len;
    }

    if (end < start) end = start;

    var buf = Buffer(end - start);
    this.copy(buf, 0, start, end);
    return buf;
};

Buffer.concat = function(list, length) {
    if (!util.isArray(list)) {
        throw new TypeError('Usage: Buffer.concat(list[, length])');
    }

    if (util.isUndefined(length)) {
        length = 0;
        for (var i = 0; i < list.length; i++) {
            length += list[i].length;
        }
    } else {
        length = length >>> 0;
    }

    if (list.length === 0) {
        return Buffer(0);
    } else if (list.length === 1) {
        return list[0];
    }

    var buffer = Buffer(length);

    var pos = 0;
    for (var i = 0; i < list.length; i++) {
        var buf = list[i];
        buf.copy(buffer, pos);
        pos += buf.length;
    }

    return buffer;
};

Duktape.Buffer.prototype.equals = function(buf) {
    return binding.compare(this.valueOf(), buf.valueOf()) === 0;
};

Duktape.Buffer.prototype.compare = function(buf) {
    return binding.compare(this.valueOf(), buf.valueOf());
};

Duktape.Buffer.prototype.forEach = function(cb) {
    var buf = this.valueOf();
    binding.foreach(buf, cb);
};

Buffer.compare = function compare(a, b) {
    return binding.compare(a.valueOf(), b.valueOf());
};


function checkOffset(offset, ext, length) {
    if (offset + ext > length)
        throw new RangeError('index out of range');
}


Duktape.Buffer.prototype.readUInt8 = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 1, this.length);
    return this[offset];
};


Duktape.Buffer.prototype.readUInt16LE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 2, this.length);
    return this[offset] | (this[offset + 1] << 8);
};


Duktape.Buffer.prototype.readUInt16BE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 2, this.length);
    return (this[offset] << 8) | this[offset + 1];
};


Duktape.Buffer.prototype.readUInt32LE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 4, this.length);

    return ((this[offset]) |
            (this[offset + 1] << 8) |
            (this[offset + 2] << 16)) +
            (this[offset + 3] * 0x1000000);
};


Duktape.Buffer.prototype.readUInt32BE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 4, this.length);

    return (this[offset] * 0x1000000) +
            ((this[offset + 1] << 16) |
            (this[offset + 2] << 8) |
            this[offset + 3]);
};


Duktape.Buffer.prototype.readInt8 = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 1, this.length);
    var val = this[offset];
    return !(val & 0x80) ? val : (0xff - val + 1) * -1;
};


Duktape.Buffer.prototype.readInt16LE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 2, this.length);
    var val = this[offset] | (this[offset + 1] << 8);
    return (val & 0x8000) ? val | 0xFFFF0000 : val;
};


Duktape.Buffer.prototype.readInt16BE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 2, this.length);
    var val = this[offset + 1] | (this[offset] << 8);
    return (val & 0x8000) ? val | 0xFFFF0000 : val;
};


Duktape.Buffer.prototype.readInt32LE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 4, this.length);

    return (this[offset]) |
            (this[offset + 1] << 8) |
            (this[offset + 2] << 16) |
            (this[offset + 3] << 24);
};


Duktape.Buffer.prototype.readInt32BE = function(offset, noAssert) {
    offset = offset >>> 0;
    if (!noAssert)
        checkOffset(offset, 4, this.length);

    return (this[offset] << 24) |
            (this[offset + 1] << 16) |
            (this[offset + 2] << 8) |
            (this[offset + 3]);
};


function checkInt(buffer, value, offset, ext, max, min) {
    // if (!(buffer instanceof Buffer))
    //     throw new TypeError('buffer must be a Buffer instance');
    if (value > max || value < min)
        throw new TypeError('value is out of bounds');
    if (offset + ext > buffer.length)
        throw new RangeError('index out of range');
}


Duktape.Buffer.prototype.writeUInt8 = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 1, 0xff, 0);
    this[offset] = value;
    return offset + 1;
};


Duktape.Buffer.prototype.writeUInt16LE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 2, 0xffff, 0);
    this[offset] = value;
    this[offset + 1] = (value >>> 8);
    return offset + 2;
};


Duktape.Buffer.prototype.writeUInt16BE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 2, 0xffff, 0);
    this[offset] = (value >>> 8);
    this[offset + 1] = value;
    return offset + 2;
};


Duktape.Buffer.prototype.writeUInt32LE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 4, 0xffffffff, 0);
    this[offset + 3] = (value >>> 24);
    this[offset + 2] = (value >>> 16);
    this[offset + 1] = (value >>> 8);
    this[offset] = value;
    return offset + 4;
};


Duktape.Buffer.prototype.writeUInt32BE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 4, 0xffffffff, 0);
    this[offset] = (value >>> 24);
    this[offset + 1] = (value >>> 16);
    this[offset + 2] = (value >>> 8);
    this[offset + 3] = value;
    return offset + 4;
};


Duktape.Buffer.prototype.writeInt8 = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 1, 0x7f, -0x80);
    this[offset] = value;
    return offset + 1;
};


Duktape.Buffer.prototype.writeInt16LE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 2, 0x7fff, -0x8000);
    this[offset] = value;
    this[offset + 1] = (value >>> 8);
    return offset + 2;
};


Duktape.Buffer.prototype.writeInt16BE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 2, 0x7fff, -0x8000);
    this[offset] = (value >>> 8);
    this[offset + 1] = value;
    return offset + 2;
};


Duktape.Buffer.prototype.writeInt32LE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 4, 0x7fffffff, -0x80000000);
    this[offset] = value;
    this[offset + 1] = (value >>> 8);
    this[offset + 2] = (value >>> 16);
    this[offset + 3] = (value >>> 24);
    return offset + 4;
};

Duktape.Buffer.prototype.writeInt32BE = function(value, offset, noAssert) {
    value = +value;
    offset = offset >>> 0;
    if (!noAssert)
        checkInt(this, value, offset, 4, 0x7fffffff, -0x80000000);
    this[offset] = (value >>> 24);
    this[offset + 1] = (value >>> 16);
    this[offset + 2] = (value >>> 8);
    this[offset + 3] = value;
    return offset + 4;
};

Buffer.isEncoding = function(encoding) {
  var loweredCase = false;
  for (;;) {
    switch (encoding) {
      case 'hex':
      case 'utf8':
      case 'utf-8':
      case 'ascii':
      case 'binary':
      case 'base64':
      case 'ucs2':
      case 'ucs-2':
      case 'utf16le':
      case 'utf-16le':
        return true;

      default:
        if (loweredCase)
            return false;
        
        encoding = ('' + encoding).toLowerCase();
        loweredCase = true;
    }
  }
};

function base64ByteLength(str, bytes) {
  // Handle padding
  if (str.charCodeAt(bytes - 1) === 0x3D)
    bytes--;
  if (bytes > 1 && str.charCodeAt(bytes - 1) === 0x3D)
    bytes--;

  // Base64 ratio: 3/4
  return (bytes * 3) >>> 2;
}

function byteLength(string, encoding) {
  if (typeof string !== 'string')
    string = '' + string;

  var len = string.length;
  if (len === 0)
    return 0;

  // Use a for loop to avoid recursion
  var loweredCase = false;
  for (;;) {
    switch (encoding) {
      case 'ascii':
      case 'binary':
        return len;

      case 'utf8':
      case 'utf-8':
      case undefined:
        return Duktape.Buffer(string).byteLength;

      case 'ucs2':
      case 'ucs-2':
      case 'utf16le':
      case 'utf-16le':
        return len * 2;

      case 'hex':
        return len >>> 1;

      case 'base64':
        return base64ByteLength(string, len);

      default:
        // The C++ binding defaulted to UTF8, we should too.
        if (loweredCase)
          return binding.byteLengthUtf8(string);

        encoding = ('' + encoding).toLowerCase();
        loweredCase = true;
    }
  }
}

Buffer.byteLength = byteLength;

exports.SlowBuffer = Duktape.Buffer;
exports.Buffer = Buffer;
