var errno = process.binding('errno');
exports.EOF = errno.EOF;

//io socket
exports.Socket  = require('./io/Socket.js');
exports.Socket2  = require('./io/Socket2.js');

// ErrShortWrite means that a write accepted fewer bytes than requested
// but failed to return an explicit error.
var ErrShortWrite = new Error("short write");

// ErrShortBuffer means that a read required a longer buffer than was provided.
var ErrShortBuffer = new Error("short buffer");

// EOF is the error returned by Read when no more input is available.
// Functions should return EOF only to signal a graceful end of input.
// If the EOF occurs unexpectedly in a structured data stream,
// the appropriate error is either ErrUnexpectedEOF or some other error
// giving more detail.
var EOF = new Error("EOF");

function _is_error (e){
    return e instanceof Error;
}

exports.Reader = function () {
    this.Read = function(){};
};

exports.Writer = function () {
    this.Write = function(){};
};

// copyBuffer is the actual implementation of Copy and CopyBuffer.
// if buf is nil, one is allocated.
function copyBuffer(dst /* Writer */, src /* Reader */, buf) {
    var written = 0;
    var err = null;

    // If the reader has a WriteTo method, use it to do the copy.
    // Avoids an allocation and a copy.
    var wt = src.WriterTo;
    if (wt) {
        return wt.WriteTo(dst);
    }

    // Similarly, if the writer has a ReadFrom method, use it to do the copy.
    var rt = dst.ReaderFrom;
    if (rt) {
        return rt.ReadFrom(src);
    }

    if (!buf) {
        buf = Buffer(32*1024);
    }

    while (1) {
        var nr = src.Read(buf);
        if (_is_error(nr)){
            if (nr !== EOF){
                err = nr;
            }
            break;
        }

        if (nr > 0) {
            var nw = dst.Write(buf, nr);
            if (_is_error(nw)){
                err = nw;
                break;
            }

            if (nw > 0) {
                written += nw;
            }

            if (nr !== nw) {
                err = ErrShortWrite;
                break
            }
        }
    }

    if (err){
        return err;
    }

    return written, err
}


// Copy copies from src to dst until either EOF is reached
// on src or an error occurs.  It returns the number of bytes
// copied and the first error encountered while copying, if any.
//
// A successful Copy returns err == nil, not err == EOF.
// Because Copy is defined to read from src until EOF, it does
// not treat an EOF from Read as an error to be reported.
//
// If src implements the WriterTo interface,
// the copy is implemented by calling src.WriteTo(dst).
// Otherwise, if dst implements the ReaderFrom interface,
// the copy is implemented by calling dst.ReadFrom(src).
exports.Copy = function(dst /* Writer */, src /* Reader */) {
    return copyBuffer(dst, src, null);
};
