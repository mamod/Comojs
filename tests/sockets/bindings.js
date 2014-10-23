var socket = process.binding('socket');
var assert = require('assert');
var errno = process.binding('errno');

var isWin = process.platform == 'win32';

//creating ip4 & ip6 addresses
(function(){
    //this will give us a pointer to sock address
    var ip4, ip6;
    
    process.errno = 0;
    ip4 = socket.pton4('127.0.0.1', 9090);
    assert.ok(ip4 !== null);
    assert.ok(!process.errno);
    
    //setting ip4 to ip6 interface
    //should set process.errno to EINVAL and return null
    ip6 = socket.pton6('127.0.0.1', 8080);
    assert.ok(ip6 === null);
    assert.equal(process.errno, errno.EINVAL);
    
    //real ip6 address
    process.errno = 0;
    ip6 = socket.pton6('FE80::0202:B3FF:FE1E:8329', 80);
    assert.ok(ip6 !== null);
    assert.equal(process.errno, 0);
    
    //pton & ip should detect ip address automatically
    ip4 = socket.pton('127.0.0.1', 9990);
    assert.ok(ip4 !== null);
    assert.equal(process.errno, 0);
    
    ip6 = socket.pton('FE80:0000:0000:0000:0202:B3FF:FE1E:8329', 9990);
    assert.ok(ip6 !== null);
    assert.equal(process.errno, 0);
    
    //wrong ip4 address
    ip4 = socket.pton('127.0.0.a', 9990);
    assert.ok(ip4 === null);
    assert.equal(process.errno, errno.EINVAL);
    
    //wrong ip6 address
    ip6 = socket.pton('FE80:0000^1E:8329', 9990);
    assert.ok(ip6 === null);
    assert.equal(process.errno, errno.EINVAL);
    
    //ports should be numbers otherwise should fail
    
})();

//creating sockets
(function(){
    
    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 6);
    assert.ok(typeof s === 'number');
    assert.ok(socket.close(s));
    assert.ok(!socket.close(9999));
    
    //should fail with EAFNOSUPPORT
    var s = socket.socket(999, socket.SOCK_STREAM, 6);
    assert.ok(s === null);
    assert.equal(process.errno, errno.EAFNOSUPPORT);
    
    /* ====================================================
     *  setting unknown type
     *  should fail with EINVAL on posix & ESOCKTNOSUPPORT
     *  on windows !! should we unify this error?!
    * ===================================================== */
    s = socket.socket(socket.AF_INET, 888, 6);
    assert.ok(s === null);
    if (isWin) {
        assert.equal(process.errno, errno.ESOCKTNOSUPPORT);
    } else {
        assert.equal(process.errno, errno.EINVAL);
    }
    
    //reset errno
    process.errno = 0;
    
    //setting unknown protocol
    //should fail EPROTONOSUPPORT
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 999);
    assert.ok(s === null);
    assert.equal(process.errno, errno.EPROTONOSUPPORT);
    
})();

//bind
(function(){
    process.errno = 0;
    
    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 6);
    assert.ok(s);
    assert.ok(typeof s == 'number');
    
    //local host ipv4
    var ip = socket.pton('127.0.0.1', 9090);
    assert.ok(ip);
    
    //reuse address
    assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR, 1));

    assert.ok(socket.bind(s, ip), process.errno); //socket is now bound to port 9090
    assert.ok(!process.errno);

    //bind to unavailable local ip address
    var not_real_ip = socket.pton('46.185.254.40', 99);
    assert.ok(not_real_ip);
    assert.ok(!socket.bind(s, not_real_ip));
    
    //FIXME: windows gives WSAEINVAL??
    assert.ok(process.errno === errno.WSAEINVAL ||
                 process.errno === errno.EADDRNOTAVAIL);
    
    assert.ok(socket.close(s));
    s = null;
    
    /*
     * check if tests platform supports ipv6 then test
     * as winxp and old platforms in general doesn't
     * support ipv6
    */
    
    s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0);
    if (s) { //no error
        var ip = socket.pton('::1', 9090); //localhost
        assert.ok(ip);

        //reuse address
        assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
                   socket.SO_REUSEADDR, 1));

        assert.ok(socket.bind(s, ip));
        console.log('ipv6 version supported');
        assert.ok(socket.close(s));
    } else {
        //ipv6 not supported
        assert.equal(process.errno, errno.EAFNOSUPPORT);
    }
    
})();


(function(){
    process.errno = 0;
    
    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    assert.ok(s);
    assert.ok(typeof s == 'number');
    
    //local host ipv4
    var ip = socket.pton('127.0.0.1', 9090);
    assert.ok(ip);

    //reuse
    assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR, 1));
    
    assert.ok(socket.bind(s, ip)); //socket is now bound to port 9090
    assert.ok(socket.listen(s, 1));
    
    var connectSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    assert.ok(socket.connect(connectSock, ip)); //connect
    assert.equal(process.errno, 0);
    
    socket.send(connectSock, "hi", 2, 0);
    
    var acceptSock = socket.accept(s);
    assert.ok(acceptSock, process.errno);
    
    var n = socket.recv(acceptSock, 1024);
    assert.strictEqual(n, 'hi');
    
    assert.ok(socket.shutdown(acceptSock, 2));
    assert.ok(socket.shutdown(connectSock, 2));
    
    assert.ok(socket.close(s));
    assert.ok(socket.close(acceptSock));
    assert.ok(socket.close(connectSock));
    assert.ok(!process.errno);
    
})();

//address info
(function(){
    
    var ip = socket.pton('127.0.0.1', 9090);
    assert.ok(ip);
    
    var address = socket.ntop(ip);
    assert.strictEqual('127.0.0.1', address);
    
    //packed ipv6 address
    ip = socket.pton('::1', 9090); //localhost
    assert.ok(ip);
    assert.strictEqual(socket.ntop4(ip), '0.0.0.0');
    assert.strictEqual(socket.ntop6(ip), '::1');
    assert.strictEqual(socket.ntop(ip), '::1');
    
    //address ipv6 will remove zeros => (compact version)
    ip = socket.pton('2002:4559:1FE2:0000:0000:0000:4559:1FE2', 9090);
    assert.ok(ip);
    address = socket.ntop(ip); //2002:4559:1fe2::4559:1fe2
    assert.strictEqual(address, '2002:4559:1fe2::4559:1fe2');
    
    //pass some fake pointer
    process.errno = 0;
    var p = Duktape.Pointer({});
    address = socket.ntop(p);
    assert.ok(!address); // error
    assert.equal(process.errno, errno.EINVAL);
    
})();

