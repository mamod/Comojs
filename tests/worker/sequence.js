var Worker = require('worker');
var assert = require('assert');

exports.onmessage = function(e){
    //response back with the same data received
    e.postMessage(e.data);
};

var expected = [ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,11,12,13,14,15,16,
                17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,
                37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,
                57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,
                77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
                97,98,99 ];

if (process.main){
    //create a worker with 10 threads pool
    var w = new Worker(__filename);
    var points = [];
    w.onmessage = function(m){
        points.push(m);
    };

    //post 100 message to workers
    for (var i = 0; i < 100; i++){
        w.postMessage(i + '');
    }
    
    //1000 delay, worker should catch this
    setTimeout(function(){
        //post another 10 messages to workers
        for (var i = 0; i < 10; i++){
            w.postMessage(i + '');
        }
    },100);

    //wait untill all workers done
    setTimeout(function(){
        //we need to sortr as data is not guranteed to 
        //arrive in the same order as they sent
        points.sort(function(a, b){return a-b});
        //console.log(points);

        assert.deepEqual(points, expected);
        w.destroy();
    }, 200);

    process.on("exit", function(d){
        print('done');
    });
}
