var socket = process.binding('socket');

exports.isIP = function(ip){
    return socket.isIP(ip);
};
