var common = require('../common');
var path = require('path');
var fs = require('fs');
var xFile = path.join(common.fixturesDir, 'x.txt');

var fd = fs.openSync(xFile, 'r');
console.log(fd);

var buffer = Buffer(3);
var buffer2 = Buffer(2);
fs.read(fd, buffer, 0, 3, -1, function(err, buf){
    if (err) throw err;
    fs.readSync(fd, buffer2, 0, 2, 1);
    console.log(buffer);
    console.log(buffer2);
    fs.closeSync(fd);
});
