var Worker = require("worker");

var i = 0;

function main() {
    var w = new Worker(__filename);
    w.onmessage = function(e) {
        print(e.data + " " + ++i);
        e.postMessage("  PING");
    };

    w.onerror = function(e) {
        throw new Error(e.data);
    }

    w.postMessage("  PING");
}

exports.onmessage = function(e) {
    
    if (++i < 100) { 
        e.postMessage("      PONG ");
        print(e.data + " " + i);
    }
}

if (process.main) {
    main();
}
