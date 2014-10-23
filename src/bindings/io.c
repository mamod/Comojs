#include <fcntl.h>

#ifdef _WIN32
    #include "../../include/ansi/ANSI.h"
    #include <io.h>
    #define GET_LAST_ERROR GetLastError()
#else
    #define GET_LAST_ERROR errno
    #include <unistd.h>
#endif

static const int _io_open(duk_context *ctx) {
    const char *filename = duk_require_string(ctx, 0);
    int flag             = duk_require_int(ctx, 1);
    int mode             = duk_require_int(ctx, 2);

    printf("ERROR : %s\n", filename);
    int fd = open(filename, flag, mode);
    if (fd < 0){
        goto error;
    }
    duk_push_int(ctx, fd);
    return 1;
    
    error:
        if (fd > 0) close(fd);
        COMO_SET_ERRNO(ctx, GET_LAST_ERROR);
        duk_push_null(ctx);
    return 1;
}

static const int _io_read(duk_context *ctx) {
    int fd     = duk_require_int(ctx, 0);
    size_t len = (size_t)duk_get_int(ctx, 1);
    //Buffer *buffer = duk_require_pointer(ctx, 2);
    //int fd2 = _open_osfhandle(fd, _O_RDONLY);
    //printf("FD : %d\n", fd2);
    
    char *buf = duk_push_fixed_buffer(ctx, len);
    size_t n = read(fd, buf, len);
    
    if (n == -1){
        printf("ERROR : %d\n", GET_LAST_ERROR);
        COMO_SET_ERRNO_AND_RETURN(ctx, GET_LAST_ERROR);
    }
    
    return 1;
}

static const int _io_write(duk_context *ctx) {
    int fd = duk_require_int(ctx, 0);
    size_t length;
    const char *str = duk_require_lstring(ctx, 1, &length);
    
    //FIXME run trhis only on tty device
    #ifdef _WIN32
        HANDLE fileHandle = (HANDLE)_get_osfhandle(fd);
        if (fileHandle == INVALID_HANDLE_VALUE){
            COMO_SET_ERRNO_AND_RETURN(ctx, GET_LAST_ERROR);
        }
        
        LPDWORD lpNumberOfBytesWritten = 0;
        BOOL ret = MyWriteConsoleW(fileHandle,
                              str,
                              (DWORD)length,
                              (LPDWORD)&lpNumberOfBytesWritten,
                              NULL);
        
        duk_push_int(ctx, (size_t)lpNumberOfBytesWritten);
        return 1;
    #endif
    
    size_t n = write(fd, str, length);
    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, GET_LAST_ERROR);
    }
    
    duk_push_int(ctx, (size_t)length);
    return 1;
}

static const duk_function_list_entry como_io_funcs[] = {
    { "read", _io_read,   3 },
    { "open", _io_open,   3 },
    { "write", _io_write, 2 },
    { NULL, NULL, 0 }
};

static const duk_number_list_entry como_io_constants[] = {
    { "O_APPEND" , O_APPEND },
    { "O_CREAT"  ,  O_CREAT },
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

static const int init_binding_io(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_io_funcs);
    duk_put_number_list(ctx,   -1, como_io_constants);
    return 1;
}
