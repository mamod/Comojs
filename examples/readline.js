var io = process.binding('tty');
var readline = require('readline');
readline.start();

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
