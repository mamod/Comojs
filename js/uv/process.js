var isWin = process.platform === 'win32';
module.exports = isWin ? require('./process/win32.js') :
                         require('./process/unix.js');
