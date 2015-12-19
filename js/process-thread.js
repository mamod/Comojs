(function(process){

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

})(process);
