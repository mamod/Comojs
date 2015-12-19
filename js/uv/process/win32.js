var assert = require('assert');
var uv     = require('uv');
var sock   = process.binding('socket');
var errno  = process.binding('errno');
var posix  = process.binding('posix');
var sys    = process.binding('sys');

var DETACHED_PROCESS           = 0x00000008;
var CREATE_NEW_PROCESS_GROUP   = 0x00000200;
var CREATE_UNICODE_ENVIRONMENT = 0x00000400;

function Process (options) {

    options = options || {};

    assert(options.file, "options.file required");
    assert(!(options.flags & ~(uv.PROCESS_DETACHED |
                               uv.PROCESS_SETGID |
                               uv.PROCESS_SETUID |
                               uv.PROCESS_WINDOWS_HIDE |
                               uv.PROCESS_WINDOWS_VERBATIM_ARGUMENTS)));

    var stdio_count = options.stdio_count;
    if (stdio_count < 3) stdio_count = 3;

    var err = errno.ENOMEM;

    var pipes = [];

    for (var i = 0; i < stdio_count; i++) {
        pipes[i]    = [];
        pipes[i][0] = -1;
        pipes[i][1] = -1;
    }

    for (var i = 0; i < options.stdio_count; i++) {
        err = this.init_stdio(options.stdio[i], pipes[i], i);
        if (err) throw new Error("STDIO initiate error " + err);
    }

    this.status = 0;
    this.pid = 0;
    this.exit_cb = options.exit_cb;

    var ret_val = this.child_init(options, stdio_count, pipes);
    if (!ret_val) {
        //error
    } else {
        this.process_handle = ret_val[0] || -1;
        this.pid = ret_val[1] || 0;
    }
    
    for (var i = 0; i < options.stdio_count; i++) {
        err = this.open_stream(options.stdio[i], pipes[i], i === 0);
        if (err === 0) continue;
        
        while (i--) {
            this.close_stream(options.stdio[i]);
        }
        throw new Error(" got error ");
    }

    //Only activate this handle if exec() happened successfully
    if ( this.pid ) {
        var self = this;
        var process_handle = this.process_handle;
        self.pipes = pipes;
        self.child_watcher = setInterval(function(){
            var exitcode = sys.GetExitCodeProcess(process_handle);
            //still running
            if (exitcode === 259){
                return;
            }
            clearInterval(this);
            if (self.exit_cb) self.exit_cb.call(self, exitcode);
        }, 1);
    }

    if (err) throw new Error("spawn error");
    return this;
}

Process.prototype.open_stream = function (container, pipefds, writable) {

    if (!(container.flags & uv.CREATE_PIPE) || pipefds[0] < 0) {
        return 0;
    }

    //already duplicated and no more needed in parent
    assert(sock.close(pipefds[0]), "closing child pipe");
    pipefds[0] = -1;

    sock.nonblock(pipefds[1], 1);

    var flags = 0;
    if (container.stream.type === 'NAMED_PIPE' && container.stream.ipc ) {
        flags = uv.STREAM_READABLE | uv.STREAM_WRITABLE;
    } else if (writable) {
        flags = uv.STREAM_WRITABLE;
    } else {
        flags = uv.STREAM_READABLE;
    }
    
    return container.stream.stream_open(pipefds[1], flags);
};

Process.prototype.child_init = function(options, stdio_count, pipes) {

    var process_flags = CREATE_UNICODE_ENVIRONMENT;
    if (options.flags & uv.PROCESS_DETACHED) {
        process_flags |= DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;
    }
    
    //initiate stdio handles
    var stdioHandles = [-1,-1,-1];

    for (var fd = 0; fd < stdio_count; fd++) {
        var use_handle   = pipes[fd][2];
        if (use_handle > 0) {
            if (fd <= 2) {
                stdioHandles[fd] = use_handle;
            } else {
                // push @shared_handles, "$fd,$use_handle";
            }
        }
    }

    // if ($options->{cwd} && !POSIX::chdir($options->{cwd})) {
    //     return;
    // }
    
    // if ($options->{env}) {
    //     %ENV = %{ $options->{env} };
    // }

    // $ENV{NODE_WIN_HANDLES} = join "#", @shared_handles;

    var processInfo = sys.CreateProcess(stdioHandles[0], stdioHandles[1], stdioHandles[2]);
    
    return processInfo;
    
    // #restore working path
    // if ($options->{cwd}) {
    //     POSIX::chdir($DIR) or return;
    // }
    
    // return \@processInfo;
}


Process.prototype.init_stdio = function (container, fds, i) {
    var mask = uv.IGNORE | uv.CREATE_PIPE | uv.INHERIT_FD | uv.INHERIT_STREAM;
    if (i <= 2) {
        // var nul = posix.open("NUL", i === 0 ? posix.O_RDONLY : posix.O_RDWR);
        // if (nul === null){
        //     throw new Error("cant' create nul file : " + process.errno);
        // }

        // //get win handle
        // var nul_handle = sys.GetOsFHandle( nul );
        // fds[2] = nul_handle;
        // assert(fds[2] !== null, "can't get win32 os fhandle");
        fds[2] = -1;
    }

    if (!container) return 0;
    switch (container.flags & mask){
        case uv.IGNORE : {
            return 0;
        }

        case uv.CREATE_PIPE : {
            assert(container.stream.type === 'NAMED_PIPE');

            if (uv.make_socketpair(fds, 0)) throw new Error("Error creating socketpair");
            var fh = fds[0];
            var inherit_handle = -1;
            if (i > 2) {
                inherit_handle = uv.make_inheritable(fh);
            } else {
                inherit_handle = uv.duplicate_handle(fh);
            }

            fds[2] = inherit_handle;
            return 0;
        }

        case uv.INHERIT_FD :
        case uv.INHERIT_STREAM : {
            if (container.flags & uv.INHERIT_FD) {
                fd = container.fd;
            } else {
                fd = container.stream.fd;
            }
            
            if (!fd || fd == -1){
                return errno.EINVAL;
            }
            
            //duplicate fd to handle
            var handle = sys.GetOsFHandle(fd);
            if (handle === null) return process.errno;
            var dupHandle = uv.duplicate_handle(handle);
            fds[2] = dupHandle;
            return 0;
        }

        default : {
            throw new Error("unknown type");
        }
    }
}

module.exports = Process;
