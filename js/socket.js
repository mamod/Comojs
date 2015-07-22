"use strict";

var socket = process.binding('socket');
var errno  = process.binding('errno');
var handle = require('handle');

var SOCK_STREAM = socket.SOCK_STREAM;
var AF_INET     = socket.AF_INET;

socket.ipAddress = socket.pton;

Object.defineProperty(socket, 'hasIPv6', {
    get : function (){
        if (typeof this.IPV6 !== 'undefined') 
            return this.IPV6;
        
        var s = this.socket(this.AF_INET6, this.SOCK_STREAM, 0);
        if (s === null){
            this.IPV6 = false;
        } else {
            this.IPV6 = true;
            this.close(s);
        }
        
        return this.IPV6;
    }
});

module.exports = socket;
