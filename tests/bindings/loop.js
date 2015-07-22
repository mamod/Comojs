var assert = require('assert');
var loop = process.binding('loop');
var main = process.main_loop;

var result = [];

/* unref handle */
(function (){
    var handle = loop.handle_init(main, function(){
        assert.fail('should not be called');
    });

    loop.handle_unref(handle);
    loop.timer_start(handle, 1000, 0);
})();

/* stop handle */
(function (){
    var handle = loop.handle_init(main, function(){
        result.push(2);
    });

    loop.timer_start(handle, 1, 0);
    loop.handle_stop(handle); /* stop handle */
    loop.handle_start(handle); /* restart */
})();

/* another loop should be called first */
(function (){
    var loop2 = loop.init();
    assert.ok(loop2 !== main);
    var handle = loop.handle_init(loop2, function(){
        result.push(1);
    });

    loop.timer_start(handle, 10, 0);
    loop.run(loop2, 0);
})();

(function (){
    var handle = loop.handle_init(main, function(){
        result.push(3);
    });

    loop.handle_call(handle);
})();

process.on('exit', function(){
    print('done');
    assert.deepEqual(result, [1,3,2]);
    console.log(result);
});
