#include "../loop/core.h"

typedef struct comoLoopHandle {
    duk_context *ctx;
    void *wrap;
    void *cb;
    void *close_cb;
} comoLoopHandle;

static int _dispatch_cb (evHandle *handle, int mask){
    comoLoopHandle *data = handle->data;
    duk_context *ctx = data->ctx;
    void *cb         = data->cb;
    
    duk_push_global_object(ctx);
    duk_push_heapptr(ctx, cb);
    
    if (handle->type == EV_IO){
        duk_push_pointer(ctx, handle); 
        duk_push_int(ctx, mask);
        duk_call(ctx, 2);
    } else {
        duk_push_pointer(ctx, handle);
        duk_call(ctx, 1);
    }

    duk_idx_t idx_top = duk_get_top_index(ctx);
    duk_pop_n(ctx, idx_top + 1);
    //dump_stack(ctx, "LOOP CALLBACK");
    return 1;
}

static int _close_cb (evHandle *handle){
    comoLoopHandle *data = handle->data;
    duk_idx_t idx_top;

    duk_context *ctx = data->ctx;
    void *close_cb   = data->close_cb;
    if (close_cb){
        duk_push_global_object(ctx);
        duk_push_heapptr(ctx, close_cb);
        duk_push_pointer(ctx, handle);
        //FIXME
        duk_call(ctx, 1);
        idx_top = duk_get_top_index(ctx);
        duk_pop_n(ctx, idx_top + 1);
    }

    free(data);
    free(handle);
    data = NULL;
    handle = NULL;
    //dump_stack(ctx, "CLOSE CALLBACK");
    return 1;
}

static const int _loop_default_loop(duk_context *ctx) {
    evLoop *loop = loop_init();
    duk_push_pointer(ctx, loop);
    return 1;
}

static const int _loop_init_loop(duk_context *ctx) {
    evLoop *loop = loop_init();
    duk_push_pointer(ctx, loop);
    return 1;
}

static const int _loop_unref(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_unref(handle);
    return 1;
}

static const int _loop_ref(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_ref(handle);
    return 1;
}

static const int _loop_handle_stop(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_stop(handle);
    return 1;
}

static const int _loop_handle_start(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_start(handle);
    return 1;
}

static const int _loop_run(duk_context *ctx) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    int type     = duk_get_int(ctx, 1);

    loop_start(loop, type);
    return 1;
}

static const int _loop_init_handle(duk_context *ctx) {
    evLoop *loop      = duk_require_pointer(ctx, 0);
    void *cb          = duk_to_pointer(ctx, 1);

    evHandle *handle  = handle_init(loop,_dispatch_cb);
    comoLoopHandle *data = malloc(sizeof(*data));
    data->ctx = ctx;
    data->cb  = cb;
    data->wrap = NULL;
    handle->data = data;
    
    duk_push_pointer(ctx, handle);
    return 1;
}

//change callback for already created handle
static const int _loop_set_handle_cb(duk_context *ctx) {
    evLoop *loop      = duk_require_pointer(ctx, 0);
    void *cb          = duk_to_pointer(ctx, 1);

    evHandle *handle  = handle_init(loop,_dispatch_cb);
    comoLoopHandle *data = handle->data;
    data->cb  = cb;
    duk_push_int(ctx, 1);
    return 1;
}

static const int _loop_timer_start(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    void *cb         = duk_to_pointer(ctx, 1);
    int timeout      = duk_get_int(ctx, 2);
    int repeat       = duk_get_int(ctx, 3);

    comoLoopHandle *data = handle->data;
    data->cb = cb;

    timer_start(handle, timeout, repeat);
    return 1;
}

static const int _loop_timer_again(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    timer_again(handle);
    return 1;
}

static const int _loop_io_start(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int fd   = duk_get_int(ctx, 1);
    int mask = duk_get_int(ctx, 2);
    io_start(handle, fd, mask);
    return 1;
}

static const int _loop_io_stop(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int mask = duk_get_int(ctx, 1);
    io_stop(handle, mask);
    return 1;
}

static const int _loop_io_active(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int mask = duk_get_int(ctx, 1);
    int i = io_active(handle, mask);
    duk_push_int(ctx, i);
    return 1;
}

static const int _loop_update_time(duk_context *ctx) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    loop_update_time(loop);
    duk_push_uint(ctx, loop->time);
    return 1;
}

static const int _loop_handle_wrap(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    void *wrap       = duk_to_pointer(ctx, 1);

    comoLoopHandle *data = handle->data;
    data->wrap = wrap;

    return 1;
}

static const int _loop_handle_unwrap(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    comoLoopHandle *data = handle->data;
    duk_push_heapptr(ctx, data->wrap);
    return 1;
}

static const duk_function_list_entry loop_funcs[] = {
    { "init_loop", _loop_init_loop, 0 },
    { "default_loop", _loop_default_loop, 0},
    { "init_handle", _loop_init_handle, 2 },
    { "set_handle_cb", _loop_set_handle_cb, 2 },
    { "timer_start", _loop_timer_start, 4},
    { "timer_again", _loop_timer_again, 1},
    { "io_start", _loop_io_start, 3 },
    { "io_stop", _loop_io_stop, 2   },
    { "io_active", _loop_io_active, 2   },
    { "run_loop", _loop_run, 2 },
    { "update_time", _loop_update_time, 1},
    { "unref", _loop_unref, 1 },
    { "ref", _loop_ref, 1 },
    { "handle_stop", _loop_handle_stop, 1 },
    { "handle_start", _loop_handle_start, 1 },
    { "handle_wrap", _loop_handle_wrap, 2},
    { "handle_unwrap", _loop_handle_unwrap, 1},
    { NULL, NULL, 0 }
};

static const duk_number_list_entry loop_constants[] = {
    { "POLLIN",  EV_POLLIN  },
    { "POLLOUT", EV_POLLOUT },
    { "POLLERR", EV_POLLERR },
    { NULL, 0 }
};

static const int init_binding_loop(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, loop_funcs);
    duk_put_number_list(ctx, -1, loop_constants);
    return 1;
}
