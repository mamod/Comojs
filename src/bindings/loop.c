#include "../loop/core.h"

static int _dispatch_cb (evHandle *handle, int mask){
    duk_context *ctx = handle->data;
    //duk_require_stack(ctx, 512);
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "process");
    duk_get_prop_string(ctx, -1, "dispatch_callbacks");
    duk_push_pointer(ctx, handle);
    if (handle->type == EV_IO){
        duk_push_int(ctx, mask);
        duk_call(ctx, 2);
    } else {
        duk_call(ctx, 1);
    }
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
    
}

static const int _loop_run(duk_context *ctx) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    int type = duk_get_int(ctx, 1);
    loop_start(loop, type);
    return 1;
}

static const int _loop_init_handle(duk_context *ctx) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    evHandle *handle  = handle_init(loop,_dispatch_cb);
    handle->data = ctx;
    duk_push_pointer(ctx, handle);
    return 1;
}

static const int _loop_timer_start(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int timeout = duk_get_int(ctx, 1);
    int repeat = duk_get_int(ctx, 2);
    timer_start(handle, timeout, repeat);
    return 1;
}

static const int _loop_io_start(duk_context *ctx) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int fd = duk_get_int(ctx, 1);
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

static const int _loop_update_time(duk_context *ctx) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    loop_update_time(loop);
    duk_push_int(ctx, (uint32_t)loop->time);
    return 1;
}

static const duk_function_list_entry loop_funcs[] = {
    { "init_loop", _loop_init_loop, 0 },
    { "default_loop", _loop_default_loop, 0},
    { "init_handle", _loop_init_handle, 1 },
    {"timer_start", _loop_timer_start, 3},
    {"io_start", _loop_io_start, 3 },
    {"io_stop", _loop_io_stop, 2   },
    { "run_loop", _loop_run, 2 },
    {"update_time", _loop_update_time, 1},
    { "unref", _loop_unref, 2 },
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
