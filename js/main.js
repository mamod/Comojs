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

    process.throwErrno = function(errno){
        errno = errno || process.errno;
        throw new Error("Errno Error " + errno + "\n" + "Errno: " + process.errno);
    };

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
            return r.require('js/node/Wrap/' + name + '.js');
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
        // if (!handle[string]) return;
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
        startup.globalTimeouts();
    }

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
        //hack process stdout
        process.stdout = {
            write : print
        };
        global.console = NativeModule.require('console');
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
        util        : 'js/util.js',
        console     : 'js/console.js',
        path        : 'js/path.js',
        assert      : 'js/assert.js',
        url         : 'js/url.js',
        querystring : 'js/querystring.js',
        events      : 'js/events.js',
        buffer      : 'js/buffer.js',
        is          : 'js/is.js',
        handle      : 'js/handle.js',
        sockets     : 'js/socket.js',
        io          : 'js/io.js',
        module      : 'js/require.js',
        threads     : 'js/threads.js',
        http_parser : 'js/http-parser.js',
        readline    : 'js/readline.js',
        socket      : 'js/socket.js',
        worker      : 'js/worker.js',
        fs          : 'js/fs.js'
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
            loop.run(main_loop, 0);
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
