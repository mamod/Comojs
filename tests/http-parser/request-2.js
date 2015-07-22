
var parser = process.binding('http-parser');

module.exports = {
    name : "chunked with trailing headers. blech.",
    raw : "POST /chunked_w_trailing_headers HTTP/1.1\r\n" +
        "Transfer-Encoding: chunked\r\n" +
        "\r\n" +
        "5\r\nhello\r\n" +
        "6\r\n world\r\n" +
        "0\r\n" +
        "Vary: *\r\n" +
        "Content-Type: text/plain\r\n" +
        "\r\n",

    should_keep_alive : true,
    message_complete_on_eof : false,
    http_major : 1,
    http_minor : 1,
    method :  parser.HTTP_POST,
    query_string : "",
    fragment : "",
    request_path : "/chunked_w_trailing_headers",
    request_url : "/chunked_w_trailing_headers",
    num_headers : 3,
    headers : { 
        "Transfer-Encoding" : "chunked",
        "Vary" : "*",
        "Content-Type" : "text/plain" 
    },
    body : "hello world"
};
