var dt = new Date;
for (var i=0; i <1000; i++){
    var buf = Buffer(1000);
    buf.fill('a');
    for (var j=0; j<100; j++){
        var s2 = buf.slice(j*10, j*10 + 10).toString('hex');
        //console.log(s2);
    }
}
var end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~4000');


dt = new Date;
for (var i=0; i <1000; i++){
    var buf = Buffer(1000);
    buf.fill('a');
    for (var j=0; j<100; j++){
        var s2 = buf.slice(j*10, j*10 + 10).toString('base64');
        //console.log(s2);
    }
}
end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~4000');


dt = new Date;
for (var i=0; i <1000; i++){
    var buf = Buffer(1000);
    buf.fill('a');
    for (var j=0; j<100; j++){
        var s2 = buf.slice(j*10, j*10 + 10).toString('utf8');
        //console.log(s2);
    }
}
end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~4000');

dt = new Date;
for (var i=0; i <1000; i++){
    var buf = Buffer(1000);
    buf.fill('م');
    for (var j=0; j<100; j++){
        var s2 = buf.slice(j*10, j*10 + 10).toString('ascii');
        //console.log(s2);
    }
}
end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~4000');


dt = new Date;
for (var i=0; i <40000; i++){
    var buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[');
    
    //ascii
    s = buf.toString('ascii');
    //console.log(s);
    
    //utf8
    s = buf.toString('utf8');
    //console.log(s);
    
    //hex
    s = buf.toString('hex');
    //console.log(s);

    //base64
    s = buf.toString('base64');
	//console.log(s);

	//binary
	s = buf.toString('binary');
    //console.log(s);
}
end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~4000');


dt = new Date;
for (var i=0; i < 60000; i++){
	var buf;
    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'utf8');
    //console.log(buf.toString());

    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'ascii');
    //console.log(buf.toString());

    buf = Buffer('3cf33cf33cf33cf3', 'base64');
    //console.log(buf.toString('hex'));

    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'hex');
    //console.log(buf.toString());

    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'binary');
}

end = +new Date - dt;
console.log('time taken: ' + end + ' expected time ~3900');

//foreach
(function(){
    var buf = Buffer(5000000);
    buf.fill('م');
    var dt = new Date;
    buf.forEach(function(c, i){});
    var end = +new Date - dt;
    console.log('time taken: ' + end + ' expected time ~2000');
})();


//with timers
(function(){
	dt = new Date;
	var counter = 0;
	setInterval(function(){
		var buf;
	    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'utf8');
	    //console.log(buf.toString());

	    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'ascii');
	    //console.log(buf.toString());

	    buf = Buffer('3cf33cf33cf33cf3', 'base64');
	    //console.log(buf.toString('hex'));

	    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'hex');
	    //console.log(buf.toString());

	    buf = Buffer('ااابببتتتتتتتتتتتتتتتتببببِِِ[[[', 'binary');
	    
	    if (counter++ === 1000) {
	    	clearInterval(this);
	    	end = +new Date - dt;
	    	console.log('time taken: ' + end + ' expected time ~1900');
		}

	}, 1);
})();
