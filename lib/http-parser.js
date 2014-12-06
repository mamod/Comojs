/**
* @module http-parser
* @author Mamod A. Mehyar
* @license MIT
******************************************************************************/
"use strict";

var parser = process.binding('http-parser');
var assert = require('assert');
var lastHeaderName;

/** @constructor
  * @alias module:http-parser
  * @param type {Constant} http parsing type [ <b>parser.REQUEST</b> |
  * <b>parser.RESPONSE</b> | <b>parser.BOTH</b> ]
  *
  * @example
  * var parser = require('http-parser');
  * var p = new parser(parser.RESPONSE);
  * 
  * var HTTPResponse = "POST /example HTTP/1.1\r\n"  +
  *                    "Host: example.com\r\n"       +
  *                    "Content-Type: text/html\r\n" +
  *                    "Content-Length: 2\r\n\r\n"   +
  *                    "Hi";
  *                    
  * var nread = p.parse(HTTPResponse);
  * console.log(p.method);  // log method type
  * console.log(p.path);    // log response path
  * console.log(p.headers); // log response headers
  * p.on_status = function(val){
  *     console.log(val);
  * };
  * 
  * @returns {Object} an http parser object
  * 
******************************************************************************/
function HTTPParser (type){
    type = type || parser.HTTP_BOTH;
    this.headers = {};
    this._headers = [];
    this.url = '';
    var pointer = parser.init(type, this);
    this.pointer = pointer;
    this.type = type;
    if (pointer === null) {
        throw new Error("out of memory while creating new Http parser");
    }
    return this;
}

/** parse http strings, on error return an instance of Error object,
  * other wise number of bytes parsed so far will be returned
  *
  * @function
  * @param type {String} http string (either a response or request string)
  *
  * @returns {Number} number of parsed bytes
  * 
******************************************************************************/
HTTPParser.prototype.execute = HTTPParser.prototype.parse = function (str){
    str = String(str); //make sure it's a string
    var len = str.length;
    var nparsed = parser.parse(this.pointer, str, len);
    if (nparsed !== len) {
        //error parsing
        //TODO : handle error
    }
    
    return nparsed;
};

/** Reinitialize parser type so you can use it on different http headers
  * [response / request] without a need to reconstruct a new http-parser
  * object
  *
  * @function
  * @param type {Constant} http parsing type [ <b>parser.REQUEST</b> |
  * <b>parser.RESPONSE</b> | <b>parser.BOTH</b> ]
  *
  * @example
  * var p = new parser(parser.RESPONSE);
  * //do some response header parsing ...
  * p.reinitialize(parser.REQUEST);
  * //do some request headers parsing ...
  *
******************************************************************************/
HTTPParser.prototype.reinitialize = function (type){
    assert(type == parser.HTTP_REQUEST ||
           type == parser.HTTP_RESPONSE);
    
    this._headers = null;
    this._headers = [];
    this.headers = {};
    this.url = '';
    this.type = type;
    parser.reinitialize(this.pointer, type);
};


/** Will be called every time the parser parses header field name
  * and should be followed by on_header_value callback
  * 
  * @ignore
  * @function on_header_field
  * @param value {String} header field name
  *
  * @returns {Number} [1|0] return 1 to continue parsing, 0 to stop the parser
  * 
******************************************************************************/
HTTPParser.prototype.on_header_field = function (value){
    this._headers.push(value);
    lastHeaderName = value;
};


/** Will be called every time the parser parses header field name
  * and should be followed by on_header_value callback
  * 
  * @ignore
  * @function on_header_value
  * @param value {String} header field name
  *
  * @returns {Number} [1|0] return 1 to continue parsing, 0 to stop the parser
  * 
******************************************************************************/
HTTPParser.prototype.on_header_value = function (value){
    this._headers.push(value);
    this.headers[lastHeaderName] = value;
};

HTTPParser.prototype.on_url = function (value){
    this.url = value;
};

HTTPParser.prototype.on_status           = function (){};
HTTPParser.prototype.on_body             = function (){};
HTTPParser.prototype.on_message_begin    = function (){};

HTTPParser.prototype.on_headers_complete = function (){
    var node_cb = this[HTTPParser.kOnHeadersComplete];
    if (node_cb){
        var message_info = {};
        message_info.headers = this._headers;

        if (this.type === parser.HTTP_REQUEST) {
            message_info.url = this.url;
            message_info.method = this.method();
        }

        // STATUS
        if (this.type === parser.HTTP_RESPONSE) {
            message_info.status_code = this.status_code;
            message_info.status_message = this.status_message;
        }

        // VERSION
        message_info.versionMajor = this.versionMajor();
        message_info.versionMinor = this.versionMinor();
        message_info.shouldKeepAlive = this.shouldKeepAlive();
        message_info.upgrade = this.upgrade();

        node_cb.call(this, message_info);
        this._headers.length = 0;
        this.headers = {};
        this.url = '';
    }
};

HTTPParser.prototype.on_message_complete = function (){};

HTTPParser.prototype.shouldKeepAlive = function (){
    return parser.http_should_keep_alive(this.pointer);
};

HTTPParser.prototype.upgrade = function (){
    return parser.http_upgrade(this.pointer);
};

HTTPParser.prototype.versionMinor = function (){
    return parser.http_minor(this.pointer);
};

HTTPParser.prototype.versionMajor = function (){
    return parser.http_major(this.pointer);
};

HTTPParser.prototype.method = function (){
    return parser.http_method(this.pointer);
};

HTTPParser.prototype.finish = function (){
    return 0;
};

HTTPParser.prototype.destroy = function (){
    parser.destroy(this.pointer);
};

/** @constant REQUEST  */ HTTPParser.REQUEST  = parser.HTTP_REQUEST;
/** @constant RESPONSE */ HTTPParser.RESPONSE = parser.HTTP_RESPONSE;
/** @constant BOTH     */ HTTPParser.BOTH     = parser.HTTP_BOTH;

//compatable with node
HTTPParser.methods = [
    'DELETE',
    'GET',
    'HEAD',
    'POST',
    'PUT'
];

HTTPParser.kOnHeaders = 0;
HTTPParser.kOnHeadersComplete = 1;
HTTPParser.kOnBody = 2;
HTTPParser.kOnMessageComplete = 3;

HTTPParser.HTTPParser = HTTPParser;

module.exports = HTTPParser;
