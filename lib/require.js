
var modulePaths = [], cache = {}, path;
var NativeModule = require('native_module');

exports.require = function(id,parent){
    if (NativeModule.exists(id)) {
        return NativeModule.require(id);
    }
    
    var module = new Module(id,parent);
    return module.load();
};

function Module(id,parent){
    this.id = id;
    this.exports = {};
    this.parent = parent || null;
    if (parent && parent.children) {
        parent.children.push(this);
    }
    
    this.filename = null;
    this.loaded = false;
    this.children = [];
    this.paths = modulePaths.slice(0);
}

Module.prototype.resolve = function(){
    
    var self = this;
    var request = this.id;
    path = require('path');
    
    var basename = path.basename(request),
    ext = path.extname(basename);
    
    var start = request.substring(0, 1)
      , searchpath
      , searchPaths = [];
    
    if (start === '.' || start === '/') {
        //relative
        searchPaths = [request];
    } else {
        if (this.parent){
            searchPaths = constructPaths(this.parent.dirname,request);
        } else {
            searchPaths = [request];
        }
    }
    
    //will return matched extension
    this.tryFiles(searchPaths,ext);
    
    if (!this.parent){
        process.mainModule = this;
        this.id = '.';
    }  
};

Module.prototype._compile = function(content,filename){
    var self = this
      , dirname = self.dirname;
    
    function require (id){
        return exports.require(id,self);
    }
    
    require.extensions = Module._extensions;
    require.main = process.mainModule;
    
    var args = [self.exports, require, self, filename, dirname];
    var wrap = NativeModule.wrap(content);
    
    var fn = process.eval(wrap, filename);
    fn.apply(self.exports, args);
};

Module.prototype.load = function(){
    var self = this;
    self.resolve();
    
    var filename = self.filename
      , dirname = self.dirname
      , parent = self.parent
      , path = require('path');
    
    if (cache[filename]){
        return cache[filename].exports;
    }
    
    cache[filename] = self;
    
    var extension = path.extname(filename) || '.js';
    if (Module._extensions[extension]) {
        Module._extensions[extension](self,filename);
    }
    
    return self.exports;
};

function constructPaths (parent,req){
    var path = require('path')
      , sep = path.sep
      , paths = parent.split(sep)
      , b = [];
    
    for (var i = paths.length; i > 0; i--){
        var cpath = paths.slice(0,i);
        b.push(cpath.join(sep) + sep + 'modules' + sep + req);
    }
    return b;
}

Module.prototype.tryFiles = function(paths,ext){
    
    var extinsions = ext ?
        [''] :
        ['','.js','.como', '.json','/index.js','/index.json'];
    
    var path = require('path')
      , binding = process;
    
    for(var x = 0; x < extinsions.length; x++){
        var extend = extinsions[x];
        for (var i = 0; i < paths.length; i++){
            var tryFile = path.resolve(this.parent ? this.parent.dirname :
                                       process.cwd(), paths[i] + extend);
            
            if (binding.isFile(tryFile)){
                this.id = this.filename = tryFile;
                this.dirname = path.dirname(tryFile);
                return ext ? ext : extend;
            }
        }
    }
    
    var e = new Error('Cant Find Module ' + this.id );
    return process.reportError(e);
};

Module._initPaths = function() {
    var isWindows = process.platform === 'win32';
    var path = require('path');
    
    if (isWindows) {
        var homeDir = process.env.USERPROFILE;
    } else {
        var homeDir = process.env.HOME;
    }
    
    var paths = [path.resolve(process.execPath, '..', '..', 'lib', 'como')];
    
    if (homeDir) {
        paths.unshift(path.resolve(homeDir, '.como_libraries'));
        paths.unshift(path.resolve(homeDir, '.como_modules'));
    }
    
    if (process.env['COMO_PATH']) {
        var splitter = isWindows ? ';' : ':';
        paths = process.env['COMO_PATH'].split(splitter).concat(paths);
    }
    
    modulePaths = paths;
    // clone as a read-only copy, for introspection.
    Module.globalPaths = modulePaths.slice(0);
};

Module._extensions = {};
Module._extensions['.js'] = function(module, filename) {
    var content = process.readFile(filename);
    module._compile(content,filename);
};

Module._extensions['.dll']  = 
Module._extensions['.so']   = 
Module._extensions['.como'] = function(module, filename) {
    var require = function (id) {
        return exports.require(id, module);
    };
    var t = process.dllOpen(filename, require);
    module.exports = t;
};

Module._initPaths();
