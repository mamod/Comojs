#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else

#endif

COMO_METHOD(como_posix_open) {
    const char *name = duk_require_string(ctx, 0);
    int flags        = duk_require_int(ctx, 1);

    int fd = open(name, flags);
    if (fd == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    }
    duk_push_int(ctx, fd);
    return 1;
}

COMO_METHOD(como_posix_read) {
    // int fd         = duk_require_int(ctx, 0);
    // size_t nbytes  = duk_require_int(ctx, 1);

    // void *buffer = duk_push_fixed_buffer(ctx, nbytes);

    // int ret = read(fd, buffer, nbytes);
    // if (ret == -1){
    //     COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_ERROR);
    // }

    // duk_to_lstring(ctx, -1, (duk_size_t)&ret);
    return 1;
}

static const duk_function_list_entry como_posix_funcs[] = {
    { "open"              ,   como_posix_open, 2},
    { "read"              ,   como_posix_read, 2},
    { NULL                , NULL, 0 }
};

static const duk_number_list_entry como_posix_constants[] = {
    { "O_RDONLY"           , O_RDONLY },
    { "O_RDWR"             , O_RDWR},
    { NULL, 0 }
};

static int init_binding_posix(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_posix_funcs);
    duk_put_number_list(ctx, -1, como_posix_constants);
    return 1;
}
