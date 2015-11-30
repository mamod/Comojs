var socket = require('socket');
var assert = require('assert');
var errno = process.binding('errno');

var isWin = process.platform == 'win32';
var IPV6 = socket.hasIPv6;

var gPORT = 9090;

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
    //don't assume free port
    var ip = socket.pton('127.0.0.1', 0);
    assert.ok(ip);
    
    //reuse address
    assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR, 1));

    //bind to free port
    assert.ok(socket.bind(s, ip), process.errno);

    var addr = socket.getsockname(s);
    var info = socket.addr_info(addr);
    assert.ok(info);

    //set global port for later use
    //this is for sure a free port
    gPORT = info[1];

    assert.ok(!process.errno);


    //bind to unavailable local ip address
    var not_real_ip = socket.pton('46.185.254.40', 99);
    assert.ok(not_real_ip);
    assert.ok(!socket.bind(s, not_real_ip));
    
    //FIXME: windows gives WSAEINVAL??
    assert.ok(process.errno === errno.EINVAL ||
                 process.errno === errno.EADDRNOTAVAIL,
                 process.errno);
    
    assert.ok(socket.close(s));
    s = null;
    
    if (IPV6) {
        s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0);
        var ip = socket.pton('::1', 0); //localhost
        assert.ok(ip);

        //reuse address
        assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
                   socket.SO_REUSEADDR, 1));

        assert.ok(socket.bind(s, ip));
        console.log('ipv6 version supported');
        assert.ok(socket.close(s));

    } else {
        //ipv6 not supported
        s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0);
        assert.equal(process.errno, errno.EAFNOSUPPORT);
        assert.ok(!socket.close(s));
    }
    
})();

//connect
(function(){
    process.errno = 0;
    
    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    assert.ok(s);
    assert.ok(typeof s == 'number');
    
    //local host ipv4
    //we will use the free global port gPORT
    var ip = socket.pton('127.0.0.1', gPORT);
    assert.ok(ip);
    
    //reuse
    assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR, 1));
    
    assert.ok(socket.bind(s, ip));
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

//getprotobyname
(function(){
    var proto = socket.getprotobyname('tcp');
    assert.ok(proto);
    assert.strictEqual(proto, socket.IPPROTO_TCP);
})();


// set/get socket options
(function(){
    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    assert.ok(s);
    var opt;

    //should throws type error
    assert.throws(function() {
        opt = socket.getsockopt(s, 'ss',
               socket.SO_REUSEADDR);
    }, TypeError);
    
    //not socket
    opt = socket.getsockopt(0, socket.SOL_SOCKET,
               socket.SO_REUSEADDR);
    assert.ok(opt === null, process.errno);

    //dummy option
    opt = socket.getsockopt(s, socket.SOL_SOCKET, 9999);
    assert.ok(opt === null, process.errno);


    opt = socket.getsockopt(s, socket.SOL_SOCKET, socket.SO_REUSEADDR);

    //option is not set
    assert.equal(opt, 0);

    //set reuse address options
    assert.ok(socket.setsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR, 1));
    

    opt = socket.getsockopt(s, socket.SOL_SOCKET,
               socket.SO_REUSEADDR);
    assert.equal(opt, 1);

    assert.ok(socket.close(s));
})();

// getpeername - getsockname - addr_info - getFamily
(function(){
    var ret;
    var bindaddr;

    var s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    assert.ok(s, process.errno);
    
    //getting sockname for unbound socket = error
    ret = socket.getsockname(s);
    //console.log(socket.addr_info(ret));
    //fails on nix?!
    // assert.ok(ret === null);
    // assert.equal(process.errno, errno.EINVAL);
    
    process.errno = 0;

    //any available port
    var addr = socket.pton('127.0.0.1', 0);
    
    assert.ok(socket.bind( s, addr ));

    bindaddr = socket.getsockname(s);
    assert.ok(bindaddr);
    assert.strictEqual(socket.ntop(bindaddr), '127.0.0.1');
    

    var addrinfo = socket.addr_info(bindaddr);
    console.log(addrinfo);
    assert(typeof addrinfo === 'object');
    assert.strictEqual(addrinfo[0], '127.0.0.1');
    assert.ok(typeof addrinfo[1] === 'number');

    //getpeername

    //listen, we need to connect to this address
    //in order to get peernamne after connecting
    assert.ok(socket.listen(s, 1));


    var connectSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0);
    
    //not connected => return null
    assert(socket.getpeername(connectSock) === null);

    //connect
    assert.ok(socket.connect(connectSock, bindaddr), process.errno); //connect

    //now we can get peerinfo
    var peer = socket.getpeername(connectSock);
    assert.ok(peer, process.errno);

    //get peer address info
    var peerinfo = socket.addr_info(peer);
    assert.ok(peerinfo);
    assert.equal(peerinfo[0], '127.0.0.1');
    assert.ok(addrinfo[1] === peerinfo[1]);


    assert.ok(socket.close(connectSock));
    assert.ok(socket.close(s));
})();

//some ipv6 tests
(function(){
    if (!IPV6) return;

    var addr = socket.pton6(socket.INADDR_ANY, 0);
    assert.ok(addr);

    var s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0);
    assert.ok(s);

    assert.ok(socket.bind(s, addr), process.errno);
    var bound = socket.getsockname(s);
    assert.ok(bound);

    var info = socket.addr_info(bound);
    console.log(info);
    assert.ok(info);

    //make sure the bound address is ipv6
    assert.ok(socket.isIP(info[0]) === 6, info);
    assert.ok(socket.close(s));
})();
