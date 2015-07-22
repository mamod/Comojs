var binding  = process.binding('fs');
var util     = require('util');
var check    = require('handle');
var path     = require('path');

var openModes = {
    "r"  : 1, /* Opens a file for reading. The file must exist. */

    "w"  : 1, /* Creates an empty file for writing. If a file with 
                 the same name already exists, its content is erased and 
                 the file is considered as a new empty file. 
                */
    
    "a"  : 1, /* Appends to a file. Writing operations, append data at 
                the end of the file. The file is created if it does not 
                exist. 
                */

    "r+" : 1, /* Opens a file to update both reading and writing. 
                The file must exist. 
                */
    
    "w+" : 1, /* Creates an empty file for both reading and writing. */
    
    "a+" : 1 /* Opens a file for reading and appending. */
};

var streamTypes = {
    'binary' : 'b',
    'text'   : 't'
};


function FileHandle (handle, file, mode){
    this.handle = handle;
    this.mode   = mode;
    this.file = file;
    this.closed = 0;

    this.close = function(){
        if (!this.handle){
            throw new Error('closing already closed file handle');
        }
        binding.close(handle);
        delete this.handle;
    };

    this.read = function(){
        if (!this.handle){
            throw new Error('reading from closed file handle');
        }
        var size;
        var stat = binding.stat(file);
        if (stat != null){
            size = stat.size;
            var buffer = Buffer(size);
            this.readIntoBuffer(buffer);
            return buffer;
        } else {
            var buffers = [];
            var pos = 0;
            process.errno = 0;
            do {
                var buffer = Buffer(1024 * 8);
                var bytes = this.readIntoBuffer(buffer);
                if (bytes){
                    pos += bytes;
                    buffers.push(buffer.slice(0, bytes));
                }
            } while(bytes === buffer.length);
            
            if ( process.errno ) {
                process.throwErrno();
            }

            return Buffer.concat(buffers, pos);
        }
    };

    this.readIntoBuffer = function(buffer, options){
        if (!options){ options = {}; }
        
        if (!options.offset) {options.offset = 0; }
        if (!options.length || options.length > buffer.length) {
            options.length = buffer.length; 
        }

        if (options.offset > buffer.length) {
            throw new Error('offset "' + options.offset + 
                             '" exceeds buffer length ' + 
                             buffer.length);
        }

        if (!util.isNumber(options.position)) {options.position = null; }

        var bytesRead = binding.readIntoBuffer(handle,
                                                buffer,
                                                options.offset,
                                                options.length,
                                                options.position);

        return bytesRead;
    };

    this.write = function(data){
        if (!this.handle){
            throw new Error('writing to closed file handle');
        }

        var ret = binding.write(handle, data);
        return ret;
    };

    this.flush = function(){
        binding.flush(handle);
    };
}

Duktape.fin(FileHandle.prototype, function (fh) {
    if (fh === FileHandle.prototype) { return; }
    if (!fh.handle) { return;  /* already freed */ }
    
    try {
        fh.close();
    } catch (e) {
        print('WARNING: finalizer failed to close filehandle ');
    }

    delete fh.handle;
});


/* exported methods */

exports.open = function(file, mode, type /* [text|binary] */){
    check.Arguments(arguments, [
        'str', 
        ['str', function(arg){ if (!openModes[arg]) return 'unsupported open mode' }]
    ], 'fs.open error');

    var file = path.resolve(file);
    var streamType = 'b'; /* binary stream by default */
    if (type){
        streamType = streamTypes[type];
        if (!streamType){
            throw new Error('unknown stream type, must be "binary" or "text"');
        }
    }

    var handle = binding.open(file, mode + streamType);
    if (handle === null){
        process.throwErrno();
    }

    return new FileHandle(handle, file, mode);
};
