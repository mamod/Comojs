this.global = this;

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
    
    var default_loop;
    
    process.reportError = function (e){
        console.log(e.stack || e);
        process.exit(1);
    };
    
    process.nextTick = function (fn){
        setTimeout(fn,0);
    }
    
    process.moduleLoadList = [];
    var binding_modules = process.bindings;
    delete process.bindings; //clean up

    var cached_bindings = {};
    var wrap_test       = /wrap/i;
    process.binding = function (name){
        
        if ( wrap_test.test(name) || name === 'uv' ){
            var r = NativeModule.require('module');
            return r.require('lib/node/Wrap/' + name + '.js');
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
        if (!handle[string]) return;
        handle[string](a, b, c, d, e);
    };

    //FIXME: quick dummy bench implementation
    var _startTime = 0;
    process.benchStart = function (){
        _startTime = Date.now();
    };

    process.benchEnd = function(){
        console.log("Bench End : " + (Date.now() - _startTime) );
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
        
        global.Buffer = NativeModule.require('buffer');
        
        startup.globalConsole();
        startup.globalLoop();
        startup.globalTimeouts();
    }
    
    startup.globalLoop = function(){
        var loop = new NativeModule.require('loop');
        default_loop = new loop(0);
    };
    
    startup.globalTimeouts = function() {
        
        global.setTimeout = function(fn,timeout) {
            //var handle2 = default_loop.init_handle();
            //default_loop.timer_start(handle2, fn, timeout, 0);
            var t = NativeModule.require('timers');
            return t.setTimeout.apply(this, arguments);
        };
        
        global.setInterval = function(fn,timeout) {
            //var handle2 = default_loop.init_handle();
            //default_loop.timer_start(handle2, fn, timeout, timeout);
            var t = NativeModule.require('timers');
            return t.setInterval.apply(this, arguments);
        };
        
        global.clearTimeout = function() {
            var t = NativeModule.require('timers');
            return t.clearTimeout.apply(this, arguments);
        };
        
        global.clearInterval = function() {
            var t = NativeModule.require('timers');
            return t.clearInterval.apply(this, arguments);
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
        //hack process stdout
        process.stdout = {
            write : print
        };
        global.console = NativeModule.require('console');
    };
    
    function NativeModule(id) {
        this.filename = id + '.js';
        this.id = id;
        this.exports = {};
        this.loaded = false;
    }
    
    NativeModule._source = {
        //node modules
        util        : 'lib/node/util.js',
        console     : 'lib/node/console.js',
        path        : 'lib/node/path.js',
        assert      : 'lib/node/assert.js',
        timers      : 'lib/node/timers.js',
        timer_wrap  : 'lib/node/wrap/timer.js',
        handle_wrap : 'lib/node/wrap/handle.js',
        _linklist   : 'lib/node/_linklist.js',
        freelist    : 'lib/node/freelist.js',
        url         : 'lib/node/url.js',

        //node stream
        stream              : 'lib/node/stream.js',
        _stream_duplex      : 'lib/node/Stream/_stream_duplex.js',
        _stream_passthrough : 'lib/node/Stream/_stream_passthrough.js',
        _stream_readable    : 'lib/node/Stream/_stream_readable.js',
        _stream_writable    : 'lib/node/Stream/_stream_writable.js',
        _stream_transform   : 'lib/node/Stream/_stream_transform.js',

        //core modules
        events      : 'lib/events.js',
        net         : 'lib/net.js',
        buffer      : 'lib/buffer.js',
        loop        : 'lib/loop.js',
        is          : 'lib/is.js',
        handle      : 'lib/handle.js',
        sockets     : 'lib/socket.js',
        io          : 'lib/io.js',
        module      : 'lib/require.js',
        threads     : 'lib/threads.js',
        http_parser : 'lib/http-parser.js',
        readline    : 'lib/readline.js',
        socket      : 'lib/socket.js',
        worker      : 'lib/worker.js',
        http        : 'lib/http.js',
    };
    
    NativeModule._cache  = {};
    
    NativeModule.require = function(id) {
        if (id == 'native_module') {
            return NativeModule;
        }
        
        var cached = NativeModule.getCached(id);
        if (cached) {
            return cached.exports;
        }
        
        if (!NativeModule.exists(id)) {
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
        try {
            default_loop.run(0);
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
