
static const int _posix_fd_sets(duk_context *ctx) {
    fd_set *fds = (fd_set *)malloc(sizeof(*fds));
    FD_ZERO(fds);
    duk_push_pointer(ctx, (void *)fds);
    return 1;
}

static const int _posix_fd_set(duk_context *ctx) {
    int fd = duk_get_int(ctx, 0);
    fd_set *fds = (void *)duk_get_pointer(ctx, 1);
    if (fds == NULL){
        duk_push_null(ctx);
        return 1;
    }
    
    FD_SET(fd, fds);
    duk_push_true(ctx);
    return 1;
}

static const int _posix_select(duk_context *ctx) {
    fd_set *rfds = (void *)duk_get_pointer(ctx, 0);
    fd_set *wfds = (void *)duk_get_pointer(ctx, 1);
    fd_set *efds = (void *)duk_get_pointer(ctx, 2);
    int ms       = duk_get_int(ctx,3);
    
    printf("MS %i\n", ms);
    struct timeval tv;
    int retval;
    tv.tv_sec = 0;
    tv.tv_usec = ms;
    
    retval = select(1024, rfds, wfds, efds, ms > -1 ? &tv : NULL);
    if (retval == -1){
        //printf("ERROR : %i\n", errno);
        COMO_SET_ERRNO(ctx, errno);
    }
    
    duk_push_int(ctx, retval);
    return 1;
}

static const int _posix_fd_isset(duk_context *ctx) {
    int fd = duk_get_int(ctx, 0);
    fd_set *fds = (void *)duk_get_pointer(ctx, 1);
    if (fds == NULL){
        duk_push_null(ctx);
        return 1;
    }
    
    if (FD_ISSET(fd, fds)){
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }
    return 1;
}

static const int _posix_fd_clr(duk_context *ctx) {
    int fd = duk_get_int(ctx, 0);
    fd_set *fds = (void *)duk_get_pointer(ctx, 1);
    if (fds == NULL){
        duk_push_null(ctx);
        return 1;
    }
    
    FD_CLR(fd,fds);
    duk_push_null(ctx);
    return 1;
}

static const duk_function_list_entry posix_funcs[] = {
    { "fd_sets", _posix_fd_sets, 0 },
    { "fd_set", _posix_fd_set, 2 },
    { "select", _posix_select, 4 },
    { "fd_isset", _posix_fd_isset, 2 },
    { "fd_clr", _posix_fd_clr, 2 },
    { NULL, NULL, 0 }
};

static const int init_binding_posix(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, posix_funcs);
    return 1;
}
