var uv     = require('uv');
var errno  = process.binding('errno');
var sock   = process.binding('socket');
var ASSERT = require('assert');

var port = 8080;
var serverType;
var server_closed;

function after_shutdown (){
    console.log("shutdown");
}

function after_write (){

}

function after_read(err, buf) {
    var nread = buf ? buf.length : 0;
    if (err) {
        /* Error or EOF */
        ASSERT.strictEqual(err, errno.EOF);

        this.shutdown(after_shutdown);
        return;
    }

    if (nread == 0) {
        /* Everything OK, but nothing read. */
        return;
    }

    console.log(buf);

    /*
    * Scan for the letter Q which signals that we should quit the server.
    * If we get QS it means close the stream.
    */

    if (!server_closed) {
        if (buf.indexOf("QS") > -1){
            this.close(on_close);
        } else if (buf.indexOf("Q") > -1){
            this.server.close(on_server_close);
            server_closed = 1;
        }
    }

    this.write(buf, after_write);
}

function on_connection(status) {

    var stream;

    if (status !== 0) {
        print("Connect error %s\n" + status);
    }
    
    ASSERT(status == 0);

    switch (serverType) {
        case "TCP":
            stream = new uv.TCP();
            break;

        case "PIPE":
            // stream = malloc(sizeof(uv_pipe_t));
            // ASSERT(stream != NULL);
            // r = uv_pipe_init(loop, (uv_pipe_t*)stream, 0);
            // ASSERT(r == 0);
            break;
        default:
        throw new Error("Bad Server Type");
    }

    /* associate server with stream */
    stream.server = this;

    this.accept(stream);

    stream.read_start(after_read);
}

function tcp_server (){

    var addr = uv.ip4_address("0.0.0.0", port);
    ASSERT(addr !== null);

    serverType = "TCP";

    var tcpServer = new uv.TCP();

    var r = tcpServer.bind(addr, 0);
    if (r) {
        /* TODO: Error codes */
        print("Bind error\n");
        return 1;
    }

    r = tcpServer.listen(sock.SOMAXCONN, on_connection);
    if (r) {
        /* TODO: Error codes */
        print("Listen error %s\n" + r);
        return 1;
    }

    return 0;
}
tcp_server();
