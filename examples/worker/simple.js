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
    w.onmessage = function(data){
        print("PARENT > this is main thread again");
        print("PARENT > some child sent me " + data);
        // w.destroy();
        w.postMessage("Hello");
        // process.sleep(1000);
    }

    //start the quest
    w.postMessage("Hello");

} else {
    console.log('CHILD > a worker thread just created');
    setInterval(function(){
        console.log(99 + 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX');
    },100);
}
