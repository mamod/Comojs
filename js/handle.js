/**
* TODO
*
* @author Mamod A. Mehyar
* @module handle
* @example
*   var handle = require('handle');
******************************************************************************/
var util = require('util');

var checkTypes = {
    'num' : function(arg){ return typeof arg === 'number'},
    'str' : function (arg){ return typeof arg === 'string' },
    'ptr' : function (arg) {
        return (typeof arg !== 'string' &&
                typeof arg !== 'number' &&
                typeof arg !== 'object' &&
                arg.toString());
    },
    'any' : function (arg) { return 1 }
};

var checkMessages = {
    'num' : 'Must be a Number',
    'str' : 'Must be a String',
    'any' : '',
    'obj' : 'Must be an Object',
    'arr' : 'Must be an Array'
};

exports.Arguments = function(args, check, errMsg){
    var _errMsg = errMsg ? '\r\n' + errMsg : '';
    var errorMessage = '';
    if (args.length < check.length) {
        errorMessage = 'Wrong arguments \r\n' +
                        _errMsg;
        
        throw new Error(errorMessage);
    }
    
    //number of passed args ok
    //check against each type
    for (var i = 0; i < args.length; i++){
        var type = check[i];
        var _check;
        if (typeof type === 'object'){
            _check = type[1];
            type = type[0];
        }

        var arg = args[i];
        var test = checkTypes[type];
        if (!test(arg)) {
            
            errorMessage = 'argument(' + i + ') '+
                            checkMessages[type] +
                            _errMsg;
            
            throw new Error(errorMessage);
        }

        if (_check){
            var error = _check(arg);
            if (error) throw new Error(error);
        }
    }
};


exports.Options = function(origin, add) {
    if (!origin) origin = {};
    var keys = Object.keys(add);
    var i = keys.length;
    while (i--) {
        if (!origin[keys[i]]){
            origin[keys[i]] = add[keys[i]];
        }
    }
    return origin;
};
