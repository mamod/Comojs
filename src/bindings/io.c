#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef _WIN32
    #include "../../include/ansi/ANSI.h"
    #include <io.h>
#else
    #include <unistd.h>
#endif

/** 
  * FIXME: io open
******************************************************************************/
COMO_METHOD(como_io_open) {
    const char *filename = duk_require_string(ctx, 0);
    int flag             = duk_require_int(ctx, 1);
    int mode             = duk_require_int(ctx, 2);

    int fd = open(filename, flag, mode);
    if (fd < 0) goto error;

    duk_push_int(ctx, fd);
    return 1;
    
    error:
        if (fd > 0) close(fd);
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);

    return 1;
}

/** 
  * io read
******************************************************************************/
COMO_METHOD(como_io_read) {
    int fd     = duk_require_int(ctx, 0);
    size_t len = (size_t)duk_get_int(ctx, 1);
    
    char *buf = duk_push_fixed_buffer(ctx, len);
    size_t n = read(fd, buf, len);
    
    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    
    return 1;
}

/** 
  * io write
******************************************************************************/
COMO_METHOD(como_io_write) {
    int fd = duk_require_int(ctx, 0);
    size_t length;
    const char *str = duk_require_lstring(ctx, 1, &length);
    
    size_t n = write(fd, str, length);
    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    
    duk_push_number(ctx, n);
    return 1;
}

/** 
  * can read
******************************************************************************/
COMO_METHOD(como_io_can_read) {
    int fd      = duk_require_int(ctx, 0);
    
    int hasTimeout  = 0;
    fd_set fds;
    struct timeval tv;
    int retval;

    if (duk_is_number(ctx, 1)) {
        hasTimeout = 1;
        int timeout = duk_require_int(ctx, 1);
        tv.tv_sec = 0;
        tv.tv_usec = timeout*1000;
    }

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    retval = select(fd + 1, &fds, NULL, NULL, hasTimeout ? &tv : NULL);

    if (retval == -1) {
       COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    } else if (retval) {
       duk_push_int(ctx, 1);
    } else {
       duk_push_int(ctx, 0);
    }

    return 1;
}

/** 
  * can write
******************************************************************************/
COMO_METHOD(como_io_can_write) {
    int fd      = duk_require_int(ctx, 0);
    
    int hasTimeout  = 0;
    fd_set fds;
    struct timeval tv;
    int retval;

    if (duk_is_number(ctx, 1)) {
        hasTimeout = 1;
        int timeout = duk_require_int(ctx, 1);
        tv.tv_sec = 0;
        tv.tv_usec = timeout*1000;
    }

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    retval = select(fd + 1, NULL, &fds, NULL, hasTimeout ? &tv : NULL);

    if (retval == -1) {
       COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    } else if (retval) {
       duk_push_int(ctx, 1);
    } else {
       duk_push_int(ctx, 0);
    }

    return 1;
}

static const duk_function_list_entry como_io_funcs[] = {
    { "read"      , como_io_read,      3 },
    { "open"      , como_io_open,      3 },
    { "write"     , como_io_write,     2 },
    { "can_read"  , como_io_can_read,  2 },
    { "can_write" , como_io_can_write, 2 },
    { NULL, NULL  , 0 }
};

static const duk_number_list_entry como_io_constants[] = {
    { "O_APPEND" , O_APPEND },
    { "O_CREAT"  , O_CREAT },
    { "O_EXCL"   , O_EXCL },
    { "O_RDONLY" , O_RDONLY },
    { "O_RDWR"   , O_RDWR },
    
    #ifdef O_SYNC
        { "O_SYNC"   , O_SYNC },
    #else
        { "O_SYNC"   , 0 },
    #endif
    
    { "O_TRUNC"  , O_TRUNC },
    { "O_WRONLY" , O_WRONLY },
    
    #ifdef O_SYMLINK
        { "O_SYMLINK" , O_SYMLINK },
    #endif
    
    { NULL, 0 }
};

static int init_binding_io(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_io_funcs);
    duk_put_number_list(ctx,   -1, como_io_constants);
    return 1;
}
