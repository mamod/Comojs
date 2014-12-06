var worker = require('worker');

exports.onmessage = function(e){
    print('CHILD > parent sent me ' + e.data);

    //now send a message back to main thread
    e.postMessage("Bye");
}

var i = 0;
if (process.main){
    print("PARENT > main thread started");

    var w = new worker(__filename);
    w.onmessage = function(e){
        print("PARENT > this is main thread again");
        print("PARENT > some child sent me " + e.data);
    }

    //start the quest
    w.postMessage("Hello");

} else {
    console.log('CHILD > a worker thread just created');
}
