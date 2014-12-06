var assert = require('assert');
var handle = require('handle');

var defaultoptions = {
    test1 : 9,
    test2 : 10,
    test3 : function(){}
};

var expected = {
    test1 : 9,
    test2 : 10,
    test3 : 8,
    test4 : 'hi'
};


function TEST (options){
    this.options = handle.Options(options, defaultoptions);
}

//empty options
var t1 = new TEST();

console.log(t1);
assert.deepEqual(t1.options, defaultoptions);

//make sure it's a deep copy
assert.ok(t1.options !== defaultoptions);


var t2 = new TEST({
    test3 : 8,
    test4 : 'hi'
});

assert.deepEqual(t2.options, expected);
