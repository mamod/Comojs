var posix = process.binding('posix');
var fs    = process.binding('fs');

exports.S_IFMT  =     posix.S_IFMT;
exports.S_IFDIR =     posix.S_IFDIR; /* directory */
exports.S_IFREG =     posix.S_IFREG; /* File */
exports.S_IFBLK =     posix.S_IFBLK; /* block device */
exports.S_IFCHR =     posix.S_IFCHR; /* charcter device */
exports.S_IFIFO =     posix.S_IFIFO; /* FIFO */

exports.O_RDONLY  =     posix.O_RDONLY;
exports.O_WRONLY  =     posix.O_WRONLY;
exports.O_CREAT   =     posix.O_CREAT;
exports.O_RDWR    =     posix.O_RDWR;
exports.O_APPEND  =     posix.O_APPEND;
exports.O_TRUNC   =     posix.O_TRUNC;
exports.O_EXCL    =     posix.O_EXCL;

if (posix.O_SYMLINK){
	exports.O_SYMLINK =     posix.O_SYMLINK;
}
