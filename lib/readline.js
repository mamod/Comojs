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
    } catch (e){
        console.log(e.stack);
    }
};

function animate () {
    setTimeout(function(){io.write(1, ('\\'))},50);
    dx = 1;
    setTimeout(function(){io.write(1, ('\x1b[' + (dx) + 'D'))},50);
    
    setTimeout(function(){io.write(1, ('-'))},150);
    setTimeout(function(){io.write(1, ('\x1b[' + (dx) + 'D'))},150);
    
    setTimeout(function(){io.write(1, ('|'))},250);
    setTimeout(function(){io.write(1, ('\x1b[' + (dx) + 'D'))},250);
    
    setTimeout(function(){io.write(1, ('/'))},350);
    setTimeout(function(){io.write(1, ('\x1b[' + (dx) + 'D'))},350);
    
    setTimeout(function(){io.write(1, ('-'))},450);
    setTimeout(function(){io.write(1, ('\x1b[' + (dx) + 'D'))},450);
}

setInterval(function(){
    animate();
},500);


//setInterval(function(){
//    io.write(1, util.inspect(process, 0, 5, true))
//},500);

module.exports = new ReadLine();
