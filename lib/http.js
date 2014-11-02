var http_parser = require('http_parser');
var util        = require('util');
var events      = require('events');
var request     = require('http_request');
var loop        = require('loop');
var socket      = process.binding('socket');


exports.Request = function (){
	return request.apply({},arguments);
};
