var Worker = require("worker");
var util   = require('util');
var assert = require('assert');

function fibo(n) {
    //process.sleep(1);
    return n > 1 ? fibo(n - 1) + fibo(n - 2) : n;
}

exports.onmessage = function(e){
    var i = e.data;
    var start = Date.now();
    var n = fibo(i);
    var ms = (Date.now() - start);
    var t = "fibonnaci number index n =" + (i) + " is: " + n + " " + ms + " ms";
    console.log(t);
    e.postMessage(ms);
}
 
function main (){
    var table = {};
    
    var start = Date.now();

    var w = new Worker(__filename, 5);
    var ms = 0;
    w.onmessage = function(e){
        ms += e.data;
    };

    for (var b = 0; b <= 30; ++b) {
        w.postMessage(b);
    }

    process.on("exit", function(){
        console.log( "TOTAL TIME : " + (Date.now() - start) );
    });
}

if (process.main) {
    main();
}
