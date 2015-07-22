var fs = require('fs');
var fh = fs.open(__dirname + "/test.txt", "r");
var t = fh.read();
console.log(t.toString());
