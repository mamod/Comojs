
fd_set rfds;
fd_set wfds;
fd_set efds;

struct timeval tv;

static const int _poll_isset(duk_context *ctx) {
    int fd = duk_get_int(ctx, 0);
    
    //printf("PEVENT : %i\n", pev);
    
    if (FD_ISSET(fd, &rfds)){
        FD_CLR(fd, &rfds);
        duk_push_true(ctx);
        return 1;
    }
    
    duk_push_false(ctx);
    return 1;
}

static const int _poll_add(duk_context *ctx) {
    int fd = duk_get_int(ctx, 0);
    int pev = duk_get_int(ctx, 1);
    
    printf("PEVENT : %i\n", pev);
    
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    
    printf("ADDING : %i\n", fd);
    FD_SET(fd, &rfds);
    
    return 1;
}

static const int _poll_poll(duk_context *ctx) {
    int timeout = duk_get_int(ctx, 0);
    printf("T : %d \n", timeout);
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    printf("POLLING\n");
    int retval = select(1024, &rfds, &wfds, &efds, timeout < 0 ? NULL : &tv);
    
    if (retval == -1){
        printf("ERROR : %d\n", GetSockError);
        COMO_FATAL_ERROR("HH");
    } else {
        //FD_ZERO(&rfds);
    }
    
    duk_push_int(ctx, retval);
    return 1;
}

static const duk_function_list_entry poll_funcs[] = {
    { "add", _poll_add, 2 },
    { "poll", _poll_poll, 1 },
    { "is_set", _poll_isset, 2 },
    { NULL, NULL, 0 }
};

static const int init_binding_poll(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, poll_funcs);
    return 1;
}
