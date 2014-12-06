var loop = require('loop');
var default_loop = loop.default_loop;
var util = require('util');
var readline = process.binding('readline');
var io = process.binding('io');

function ReadLine () {
    return this;
}

ReadLine.prototype.start = function (){
    if (this.started) {
        return;
    }
    this.start = true;
    readline.start(this);
};

ReadLine.prototype.online = function (line){
    try {
        var t = eval(line);
        if (t){ io.write(1, util.inspect(t, 1, 5, true)) };
        Duktape.gc();
    } catch (e){
        console.log(e.stack);
    }
};

module.exports = new ReadLine();
