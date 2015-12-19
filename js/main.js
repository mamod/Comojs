this.global = this;

global.DTRACE_NET_SERVER_CONNECTION = function(){};
global.LTTNG_NET_SERVER_CONNECTION  = function(){};
global.COUNTER_NET_SERVER_CONNECTION = function(){};
global.COUNTER_NET_SERVER_CONNECTION_CLOSE = function(){};
global.DTRACE_NET_STREAM_END = function(){};
global.LTTNG_NET_STREAM_END = function(){};

//definegetter polufill
if (typeof Object.prototype.__defineGetter__ === 'undefined') {
    Object.defineProperty(Object.prototype, '__defineGetter__', {
        value: function (n, f) {
            Object.defineProperty(this, n, { enumerable: true, configurable: true, get: f });
        }, writable: true, enumerable: false, configurable: true
    });
}

// Number.isFinite polyfill
// http://people.mozilla.org/~jorendorff/es6-draft.html#sec-number.isfinite
if (typeof Number.isFinite !== 'function') {
    Number.isFinite = function isFinite(value) {
        // 1. If Type(number) is not Number, return false.
        if (typeof value !== 'number') {
            return false;
        }
        // 2. If number is NaN, +∞, or −∞, return false.
        if (value !== value || value === Infinity || value === -Infinity) {
            return false;
        }
        // 3. Otherwise, return true.
        return true;
    };
}

(function(process){
    'use strict';

    process.throwErrno = function(errno){
        errno = errno || process.errno;
        throw new Error("Errno Error " + errno + "\n" + "Errno: " + process.errno);
    };

    process.reportError = function (e){
        print(e.stack || e);
        process.exit(1);
    };

    process.moduleLoadList = [];
    var binding_modules = process.bindings;
    delete process.bindings; //clean up

    var cached_bindings = {};
    var wrap_test       = /_wrap/i;
    process.binding = function (name){
        if ( wrap_test.test(name) || name === 'uv' ){
            var r = NativeModule.require('module');
            return r.require('js/node/wrap/' + name + '.js');
        } else if (name === 'http_parser'){
            return NativeModule.require('http_parser');
        }

        var binding_fun = binding_modules[name];
        if (!binding_fun) {
            throw new Error('unknown binding name ' + name);
        }

        if (!cached_bindings[name]) {
            cached_bindings[name] = binding_fun();
        }
        return cached_bindings[name];
    };

    process.MakeCallback = function (handle, string, a, b, c, d, e) {
        // process._tickCallBack();
        if (!handle[string]) return;
        handle[string](a, b, c, d, e);
    };

    function startup () {
        //in case of using tcc by running como.bat the first argument will
        //be main.c so we need to replace this
        if (process.argv[0] == 'src/main.c') {
            process.argv[0] = 'como.bat';
        }

        var EventEmitter = NativeModule.require('events').EventEmitter;
        process.__proto__ = Object.create(EventEmitter.prototype, {
            constructor: {
                value: process.constructor
            }
        });

        EventEmitter.call(process);

        var path = NativeModule.require('path');
        var execPath = path.resolve(process.cwd() + '/' + process.argv[0]);
        process.execPath = execPath;
        global.DUK_Buffer = Buffer;
        global.Buffer = NativeModule.require('buffer').Buffer;
        startup.nextTick();
        startup.globalTimeouts();
        // startup.processStdio();
        startup.globalConsole();
    }

    startup.nextTick = function(){
        var nextTickQueue = [];

        var kLength   = 0;
        var kIndex    = 0;
        var tock, callback, args;

        var tickDone = function(){
            if (kLength !== 0) {
                if (kLength <= kIndex) {
                    nextTickQueue = [];
                    nextTickQueue.length = 0;
                    kLength = 0;
                } else {
                    nextTickQueue.splice(0, kIndex);
                    kLength = nextTickQueue.length;
                }
            }
            kIndex = 0;
        };

        var slice = Array.prototype.slice;
        process.nextTick = function (){
            var args = slice.call(arguments);
            var callback = args.shift();

            // on the way out, don't bother. it won't get fired anyway.
            if (process._exiting) return;

            // if (kLength > 500){
            //     process._tickCallBack();
            // }
            
            kLength++;
            nextTickQueue.push({
                callback : callback,
                args     : args
            });
        };

        process._tickCallBack = function(){
            while (kIndex < kLength){
                var tock = nextTickQueue[kIndex++];
                var callback = tock.callback;
                var args = tock.args;
                var threw = true;
                try {
                    switch(args.length){
                        case 0  : callback(); break;
                        case 1  : callback(args[0]); break;
                        case 2  : callback(args[0], args[1]); break;
                        case 3  : callback(args[0], args[1], args[2]); break;
                        case 4  : callback(args[0], args[1], args[2], args[3]); break;
                        default : callback.apply(null, args);
                    }
                    threw = false;
                } finally {
                    if (threw) tickDone();
                }
            }
            tickDone();
        };
    };

    startup.globalTimeouts = function() {
        var loop = process.binding('loop');
        var _default = loop.init();
        process.main_loop = _default;

        global.setTimeout = function(fn, timeout, a, b, c) {
            var h = loop.handle_init(_default, fn);
            fn.timerHandle = h;
            loop.timer_start(h, timeout, 0);
            return fn;
        };

        global.setInterval = function(fn, timeout) {
            var h = loop.handle_init(_default, fn);
            fn.timerHandle = h;
            loop.timer_start(h, timeout, timeout);
            return fn;
        };

        global.clearTimeout = global.clearInterval = function(timer) {
            if (!timer.timerHandle){
                throw new Error("clearing Timer Error");
            }
            loop.handle_close(timer.timerHandle);
            timer.timerHandle = null;
        };

        global.setImmediate = function() {
            var t = NativeModule.require('timers');
            return t.setImmediate.apply(this, arguments);
        };

        global.clearImmediate = function() {
            var t = NativeModule.require('timers');
            return t.clearImmediate.apply(this, arguments);
        };
    };

    startup.globalConsole = function() {
        global.console = NativeModule.require('console');
    };

    startup.processStdio  = function() {

        var stdio = NativeModule.require('./setup/stdio.js');
        var stdin, stdout, stderr;

        //stdout
        process.__defineGetter__('stdout', function() {
            if (stdout) return stdout;
            stdout = stdio.createWritableStdioStream(1);
            stdout.destroy = stdout.destroySoon = function(er) {
                er = er || new Error('process.stdout cannot be closed.');
                stdout.emit('error', er);
            };
          
            if (stdout.isTTY) {
                process.on('SIGWINCH', function() {
                    stdout._refreshSize();
                });
            }
            return stdout;
        });

        //stderr
        process.__defineGetter__('stderr', function() {
            if (stderr) return stderr;
            stderr = stdio.createWritableStdioStream(2);
            stderr.destroy = stderr.destroySoon = function(er) {
                er = er || new Error('process.stderr cannot be closed.');
                stderr.emit('error', er);
            };
            
            if (stderr.isTTY) {
                process.on('SIGWINCH', function() {
                    stderr._refreshSize();
                });
            }
            return stderr;
        });

        //stdin
        process.__defineGetter__('stdin', function() {
            if (stdin) return stdin;
            stdin = stdio.createReadableStdioStream(0);
            return stdin;
        });
    };

    var NativeModule = process.NativeModule = function(id) {
        this.filename = NativeModule._source[id];// + '.js';
        this.id = id;
        this.exports = {};
        this.loaded = false;
    }

    NativeModule._source = {
        //core modules
        loop        : 'js/loop.js',
        buffer      : 'js/buffer.js',
        buffer2     : 'buffer/buffer.js',
        is          : 'js/is.js',
        handle      : 'js/handle.js',
        sockets     : 'js/socket.js',
        io          : 'js/io.js',
        module      : 'js/require.js',
        threads     : 'js/threads.js',
        http_parser : 'js/http-parser.js',
        socket      : 'js/socket.js',
        worker      : 'js/worker.js',
        // fs          : 'js/fs.js',
        uv          : 'js/uv.js',

        //node modules
        'internal/util'     : 'js/node/internal/util.js',
        util                : 'js/node/util.js',
        path                : 'js/node/path.js',
        fs                  : 'js/node/fs.js',
        url                 : 'js/node/url.js',
        readline            : 'js/node/readline.js',
        querystring         : 'js/node/querystring.js',
        assert              : 'js/node/assert.js',
        console             : 'js/node/console.js',
        constants           : 'js/node/constants.js',
        events              : 'js/node/events.js',
        timers              : 'js/node/timers.js',
        net                 : 'js/node/net.js',
        dns                 : 'js/node/dns.js',
        cluster             : 'js/node/cluster.js',
        string_decoder      : 'js/node/string_decoder.js',
        tty                 : 'js/node/tty.js',
        stream              : 'js/node/stream.js',
        _stream_readable    : 'js/node/stream/_stream_readable.js',
        _stream_writable    : 'js/node/stream/_stream_writable.js',
        _stream_duplex      : 'js/node/stream/_stream_duplex.js',
        _stream_transform   : 'js/node/stream/_stream_transform.js',
        _stream_passthrough : 'js/node/stream/_stream_passthrough.js'
    };

    NativeModule._cache  = {};

    NativeModule.require = function(id, p) {
        if (id == 'native_module') {
            return NativeModule;
        }
        
        var cached = NativeModule.getCached(id);
        if (cached) {
            return cached.exports;
        }
        
        if (!NativeModule.exists(id)) {
            var t = NativeModule.require('module');
            var path = NativeModule.require('path');
            
            return t.require(path.resolve(process.cwd() + '/js/' + id));
            throw new Error('No such native module ' + id);
        }
        
        process.moduleLoadList.push('NativeModule ' + id);
        var nativeModule = new NativeModule(id);
        nativeModule.cache();
        nativeModule.compile();
        return nativeModule.exports;
    };

    NativeModule.getCached = function(id) {
        return NativeModule._cache[id];
    };

    NativeModule.exists = function(id) {
        return NativeModule._source.hasOwnProperty(id);
    };

    NativeModule.getSource = function(id) {
        var filename = NativeModule._source[id];
        return process.readFile(filename);
    };

    NativeModule.wrap = function(script) {
        return NativeModule.wrapper[0] + script + NativeModule.wrapper[1];
    };

    NativeModule.wrapper = [
        '(function (exports, require, module, __filename, __dirname) {' +
            'var fn = (function(){',
            //source here
            '\n});' +
            'try {' +
                'fn();\n' +
            '} catch (e){\n' +
                'process.reportError(e);\n' +
            '};\n'+
        '});'
    ];

    NativeModule.prototype.compile = function() {
        var source = NativeModule.getSource(this.id);
        source = NativeModule.wrap(source);
        var fn = process.eval(source, NativeModule._source[this.id]);
        try {
            fn(this.exports, NativeModule.require, this, this.filename);
        } catch(e){
            process.exit(1);
        }
        this.loaded = true;
    };

    NativeModule.prototype.cache = function() {
        NativeModule._cache[this.id] = this;
    };

    startup();

    var _run_looop = function(){
        var main_loop = process.main_loop;
        var loop = process.binding('loop');
        var gcHandle = loop.handle_init(main_loop, function(){
            Duktape.gc();
        });
        loop.handle_unref(gcHandle);
        loop.timer_start(gcHandle, 5000, 5000);

        try {
            var i = 0;
            var n = 0;
            while(1){
                
                for (i = 0; i < 16; i++){
                    process._tickCallBack();
                    n = loop.run(main_loop, 1);
                    if (n == 0) break;
                }

                process._tickCallBack();
                n = loop.run(main_loop, 1);
                if (n == 0) break;
                process.sleep(1);
            }

            // loop.run(main_loop, 0);
            process.emit('exit');
        } catch (e){
            process.reportError(e);
        }
    };

    var r = NativeModule.require('module');
    var argv = process.argv;
    if (argv[1]) {
        if (argv[2] && argv[2] === '--childWorker'){
            process.child = true;
            try {
                var worker = r.require('worker');
                return worker.runChild(argv[1]);
            } catch(e){
                process.reportError(e);
            }
        } else {
            process.main = true;
            r.require(process.argv[1]);
        }
    }

    _run_looop();
})(process);
