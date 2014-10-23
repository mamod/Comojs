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
    this.url = '';
    this.method = 0;
    var pointer = parser.init(type, this);
    this.pointer = pointer;
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
HTTPParser.prototype.parse = function (str){
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
    
    this.headers = {};
    this.url = '';
    this.method = 0;
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
    this.headers[lastHeaderName] = value;
};

HTTPParser.prototype.on_url = function (value){
    this.url = value;
};

HTTPParser.prototype.on_status           = function (){};
HTTPParser.prototype.on_body             = function (){};
HTTPParser.prototype.on_message_begin    = function (){};
HTTPParser.prototype.on_headers_complete = function (){};
HTTPParser.prototype.on_message_complete = function (){};

/** @constant REQUEST  */ HTTPParser.REQUEST  = parser.HTTP_REQUEST;
/** @constant RESPONSE */ HTTPParser.RESPONSE = parser.HTTP_RESPONSE;
/** @constant BOTH     */ HTTPParser.BOTH     = parser.HTTP_BOTH;

module.exports = HTTPParser;
