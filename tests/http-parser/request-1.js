var parser = process.binding('http-parser');
module.exports = {
    name : "curl get",
    type : parser.HTTP_REQUEST,
    raw  : "GET /test HTTP/1.1\r\n" +
    "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n" +
    "Host: 0.0.0.0=5000\r\n" +
    "Accept: */*\r\n" +
    "\r\n",

    should_keep_alive : true,
    message_complete_on_eof : false,
    http_major :1,
    http_minor : 1,
    method : parser.HTTP_GET,
    query_string : "",
    fragment : "",
    request_path : "/test",
    request_url : "/test",
    num_headers : 3,
    headers : {
        "User-Agent" : "curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1",
        "Host" : "0.0.0.0=5000",
        "Accept" : "*/*"
    },

    body : ""
};
