
var Server = require('./net/server.js');

exports.Server = function (){
    return Server.apply({},arguments);
};

exports.Client = function (){
    return Server.apply({},arguments);
};
