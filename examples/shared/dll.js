var dll = require('./dll.como');
exports.hi = function(){
    print('Hi called from c');
};

exports.bye = function(){
    print('Bye called from c');
};

console.log(dll.call_me());
