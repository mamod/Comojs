var assert = require('assert');

(function(){
    var buf = Buffer(901);
    buf.fill('๟'); //3-bytes
    var i = 0;
    buf.forEach(function(c){
        //print(c);
        assert(Buffer(c).toString('hex') === 'e0b99f');
        i++;
    });

    assert.equal(i, 300);
})();


(function(){
    var buf = Buffer(991);
    buf.fill('๟م'); // 3-bytes + 2-bytes

    var dt = new Date;
    var i = 0;
    var x = 0;
    buf.forEach(function(c, y){
        //print(c);
        

        if (i % 2){
            assert.equal(Buffer(c).toString('hex'), 'd985');
        } else {
            assert.equal(Buffer(c).toString('hex'), 'e0b99f');
        }

        assert.equal(i++, y);
    });

    //assert.equal(x+1, i);
    assert.equal(i, 396);
})();

(function(){
    var buf = Buffer(100);
    buf.fill('a'); // 1-bytes
    var x = 0;
    buf.forEach(function(c, i){
        //print(c);
        print("x : " + x);
        print("i : " + i);

        assert.equal(i, x);

        if (i === 5){
            return false;
        } else {
            x++;
        }
        assert(i < 5);
    });

    assert.equal(x, 5);
})();
