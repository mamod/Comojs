/**
* <p>This module provides file and path related functionality as
* defined by the <a href="http://wiki.commonjs.org/wiki/Filesystem/A">CommonJS
* Filesystem/A</a> proposal.
*
* The "fs" module provides a file system API for the manipulation of paths,
* directories, files, links, and the construction of file streams.
*
* @module Sockets
* @example
*   var socket = require('socket');
******************************************************************************/
"use strict";

var sock = process.binding('socket');
var handle = require('handle');

/** return a <b>pointer</b> to a packed ip address, this function accept
   * both type of ip address ipv4 or ipv6
   *
   * @static
   * @function
   * @param {String} ip IPv4 or IPv6 ip address
   * @param {Number} port port number
   * 
   * @example
   * //create a packed ipv4 address
   * var ip4 = sock.pton('127.0.0.1', 9090);
   *
   * @example
   * //create a packed ipv6 address
   * var ip6 = sock.pton('FE80::0202:B3FF:FE1E:8329', 8080);
   *   
   * @returns {Pointer} a pointer to a packed ip address struct
   * 
*/ exports.pton = function (ip,port) {
    
    handle.Arguments(arguments, ['str', 'num'],
                     "USAGE: sock.pton('ip', port)");
    
    var p = sock.pton(ip,port);
    if (p === null) {
        return new Error('Error');
    }
    
    return p;
}


/** Takes a packed ip address (pointer) and return a readable form of the
   * address
   *
   * @static
   * @function
   * @param {Pointer} 
   * @param {Number} port port number
   * 
   * @example
   *   //create a packed ipv4 address
   *   var ip4 = sock.pton('127.0.0.1', 9090);
   *
   * @example
   *   //create a packed ipv6 address
   *   var ip = sock.pton('127.0.0.1', 8080);
   *   var address = sock.ntop(ip);
   *   console.log(address); //127.0.0.1
   *   
   * @returns {String} a readable form of the packed ip address
   *
*/ exports.ntop = function (ip){
    handle.Arguments(arguments, ['ptr'],
                     "USAGE: sock.ntop(ip)");
    
    var address = sock.ntop(ip);
    if (address === null) {
        return new Error();
    }
    
    return address;
};


function Socket (args) {
    //code
}


/** @constant HTTP  */  exports.HTTP2 = 1;
/** @constant HTTP2 */  exports.HTTP2 = 1;
