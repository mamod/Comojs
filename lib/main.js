this.global = this;

(function(process){
    'use strict';
    
    var default_loop;
    
    process.moduleLoadList = [];
    var binding_modules = process.bindings;
    delete process.bindings;
    
    process.reportError = function (e){
        console.log(e.stack || e);
        process.exit(1);
    };
    
    process.nextTick = function (fn){
        setTimeout(fn,0);
    }
    
    var cached_bindings = {};
    process.binding = function (name){
        var binding_fun = binding_modules[name];
        if (!binding_fun) {
            throw new Error('unknown binding name ' + name);
        }
        
        if (!cached_bindings[name]) {
            cached_bindings[name] = binding_fun();
        }
        return cached_bindings[name];
    };
    
    process.MakeCallback = function (handle, string) {
        handle[string]();
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
        
        global.Buffer = NativeModule.require('buffer').Buffer;
        
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
        
        global.setInterval = function() {
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
        buffer      : 'lib/node/buffer.js',
        console     : 'lib/node/console.js',
        path        : 'lib/node/path.js',
        assert      : 'lib/node/assert.js',
        events      : 'lib/events.js',
        timers      : 'lib/node/timers.js',
        timer_wrap  : 'lib/node/wrap/timer.js',
        handle_wrap : 'lib/node/wrap/handle.js',
        _linklist   : 'lib/node/_linklist.js',
        
        //core modules
        loop        : 'lib/loop.js',
        is          : 'lib/is.js',
        handle      : 'lib/handle.js',
        sockets     : 'lib/socket.js',
        io          : 'lib/io.js',
        module      : 'lib/require.js',
        threads     : 'lib/threads.js',
        http_parser : 'lib/http-parser.js',
        readline    : 'lib/readline.js',
        slowbuffer  : 'lib/slowbuffer.js',
        http        : 'lib/http.js',
        http_request: 'lib/http/request.js'
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
        } catch (e){
            process.reportError(e);
        }
    };
    
    var r = NativeModule.require('module');
    //EIXME : very ugly thread implementation
    if (process.argv[0] == '--thread') {
        process.processLoopid = default_loop.pointer;
        //must be overriden by worker script
        process.ondata = function(){
            print('worker does not has a listener');
        };
        var e = 0;
        //FIXME: should hide this
        process.processthreadData = function (data){
            var ret;
            try {
                ret = process.ondata(data);
            } catch (e){
                process.reportError(e);
            }
            
            //_run_looop();
            return JSON.stringify({
                data : ret
            });
        }
    } else {
        process.processthreadData = function (data){};
    }
    
    if (process.argv[1]) {
        r.require(process.argv[1]);
    }
    
    _run_looop();
    try {
        process.emit('exit');
    } catch (e){
        process.reportError(e);
    }
    

})(process);
