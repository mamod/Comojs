#include "../loop/core.h"

evLoop *como_main_loop(duk_context *ctx) {
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "process");
    duk_get_prop_string(ctx, -1, "main_loop");
    evLoop *loop = duk_require_pointer(ctx, -1);
    duk_pop_n(ctx, 3);
    return loop;
}

static int como_loop_handle_close(duk_context *ctx);

static int _handle_close_callback (evHandle *handle) {

    duk_context *ctx = handle->loop->data;

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoHandles");
    duk_push_sprintf(ctx, "close_%lf", (double) handle->id);
    duk_get_prop(ctx, -2);

    duk_push_pointer(ctx, handle);
    duk_call(ctx, 1);

    //pop function result
    duk_pop(ctx);
    
    //delete callback reference
    duk_push_number(ctx, (double) handle->id);
    duk_del_prop(ctx, -2);

    //delete close_callback reference
    duk_push_sprintf(ctx, "close_%lf", (double) handle->id);
    duk_del_prop(ctx, -2);

    duk_pop_2(ctx);

    return 1;
}

static int _handle_dispatch_cb (evHandle *handle, int mask){

    duk_context *ctx = handle->loop->data;

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoHandles");
    duk_push_number(ctx, (double) handle->id);
    duk_get_prop(ctx, -2);
    
    if (handle->type == EV_IO){
        duk_push_pointer(ctx, handle);
        duk_push_int(ctx, mask);
        duk_call(ctx, 2);
    } else {
        duk_dup(ctx, -1);
        duk_push_pointer(ctx, handle);
        duk_call_method(ctx, 1);
    }

    duk_idx_t idx_top = duk_get_top_index(ctx);
    duk_pop_n(ctx, idx_top + 1);

    if (handle->type == EV_TIMER){
        evTimer *timer = handle->ev;
        if (timer->repeat == 0){
            duk_push_pointer(ctx, handle);
            como_loop_handle_close(ctx);
        }
    }
    return 1;
}

/*=============================================================================
  initate new loop
  loop.init();
 ============================================================================*/
COMO_METHOD(como_loop_init) {
    evLoop *loop = loop_init();
    loop->data = ctx;
    duk_push_pointer(ctx, loop);
    return 1;
}

/*=============================================================================
  get main loop
  loop.main();
 ============================================================================*/
COMO_METHOD(como_loop_main) {
    evLoop *loop = main_loop();
    loop->data = ctx;
    duk_push_pointer(ctx, loop);
    return 1;
}

/*=============================================================================
  start running loop
  loop.run(loop_pointer, [0|1]);
 ============================================================================*/
COMO_METHOD(como_loop_run) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    int type     = duk_get_int(ctx, 1); /* || 0 */

    int active = loop_start(loop, type);
    duk_push_int(ctx, active);
    return 1;
}


/* handle functions */

/*=============================================================================
  initaite new handle
  loop.handle_init(loop_pointer, callback);
 ============================================================================*/
COMO_METHOD(como_loop_handle_init) {
    evLoop *loop      = duk_require_pointer(ctx, 0);

    evHandle *handle  = handle_init(loop, _handle_dispatch_cb);
    int64_t handle_id = handle->id;

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoHandles");
    duk_push_number(ctx, (double) handle_id);
    duk_dup(ctx, 1);
    duk_put_prop(ctx, -3); /* comoHandles[handle_id] = callback */

    duk_push_pointer(ctx, handle);
    return 1;
}

/*=============================================================================
  associate javascript object/function with handle
  loop.handle_wrap(handle_pointer, obj);
 ============================================================================*/
COMO_METHOD(como_loop_handle_wrap) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    void *this       = duk_require_heapptr(ctx, 1);
    handle->data  = this;
    return 1;
}

/*=============================================================================
  get javascript object/function associated with handle
  loop.handle_unwrap(handle_pointer);
 ============================================================================*/
COMO_METHOD(como_loop_handle_unwrap) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    void *this      = handle->data;
    duk_push_heapptr(ctx, this);
    return 1;
}

/*=============================================================================
  handle call on next call immediately
  loop.handle_call(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_call) {
    evHandle *handle      = duk_require_pointer(ctx, 0);
    handle_call(handle);
    return 1;
}

/*=============================================================================
  unref handle
  loop.handle_unref(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_unref) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_unref(handle);
    return 1;
}

/*=============================================================================
  ref handle, ref unrefed handle
  loop.handle_ref(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_ref) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_ref(handle);
    return 1;
}

/*=============================================================================
  start inactive handle or previous stopped handle
  loop.handle_start(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_start) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_start(handle);
    return 1;
}

/*=============================================================================
  stop handle, stopping the handle doesn't close it, so you can start it 
  again by doing handle_start, to stop handle for good, use handle_close
  
  loop.handle_stop(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_stop) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    handle_stop(handle);
    return 1;
}

/*=============================================================================
  close handle, this will close handle and free al it's resources, closed 
  handles can't be restarted again.
  
  loop.handle_close(handle);
 ============================================================================*/
COMO_METHOD(como_loop_handle_close) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    
    if (duk_is_function(ctx, 1)){
        duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, "comoHandles");
        duk_push_sprintf(ctx, "close_%lf", (double) handle->id);
        duk_dup(ctx, 1);
        duk_put_prop(ctx, -3); /* comoHandles[close_handle_id] = callback */
        handle->close = (void *)_handle_close_callback;
    } else {
        /* remove handle reference from global stash */
        duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, "comoHandles");
        duk_push_number(ctx, (double) handle->id);
        duk_del_prop(ctx, -2);
    }

    handle_close(handle);
    return 1;
}



/* timer functions */

/*=============================================================================
  start timer
  loop.timer_start(handle, timeout, repeat);
 ============================================================================*/
COMO_METHOD(como_loop_timer_start) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int timeout      = duk_get_int(ctx, 1);
    int repeat       = duk_get_int(ctx, 2);

    timer_start(handle, timeout, repeat);
    return 1;
}

/*=============================================================================
  start timer again
  loop.timer_again(handle);
 ============================================================================*/
COMO_METHOD(como_loop_timer_again) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    timer_again(handle);
    return 1;
}


/* IO functions */

/*=============================================================================
  io start
  loop.io_start(handle, [loop.POLLIN | loop.POLLOUT | loop.POLLERR]);
 ============================================================================*/
COMO_METHOD(como_loop_io_start) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int fd   = duk_get_int(ctx, 1);
    int mask = duk_get_int(ctx, 2);
    io_start(handle, fd, mask);
    return 1;
}

/*=============================================================================
  io stop : stop watching io handle for some or all events
  loop.io_stop(handle, [loop.POLLIN | loop.POLLOUT | loop.POLLERR]);
 ============================================================================*/
COMO_METHOD(como_loop_io_stop) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int event = duk_get_int(ctx, 1);
    io_stop(handle, event);
    return 1;
}

COMO_METHOD(como_loop_io_remove) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int event = duk_get_int(ctx, 1);
    io_remove(handle, event);
    return 1;
}

/*=============================================================================
  io active : check if io handle is active for the given event
  loop.io_active(handle, event); => 1 if active 0 otherwise
 ============================================================================*/
COMO_METHOD(como_loop_io_active) {
    evHandle *handle = duk_require_pointer(ctx, 0);
    int event = duk_get_int(ctx, 1);
    int i = io_active(handle, event);
    duk_push_int(ctx, i);
    return 1;
}

/*=============================================================================
  update time : update loop timer internally
  loop.update_time(loop); => return current updated time
 ============================================================================*/
COMO_METHOD(como_loop_update_time) {
    evLoop *loop = duk_require_pointer(ctx, 0);
    loop_update_time(loop);
    duk_push_uint(ctx, loop->time);
    return 1;
}

static const duk_function_list_entry loop_funcs[] = {
    { "init", como_loop_init, 0 },
    { "main", como_loop_main, 0 },
    { "run", como_loop_run,   2 },

    /* handle functions */
    { "handle_init",    como_loop_handle_init,   2 },
    { "handle_unref",   como_loop_handle_unref,  1 },
    { "handle_ref",     como_loop_handle_ref,    1 },
    { "handle_stop",    como_loop_handle_stop,   1 },
    { "handle_start",   como_loop_handle_start,  1 },
    { "handle_close",   como_loop_handle_close,  2 },
    { "handle_call",    como_loop_handle_call,   1 },
    { "handle_wrap",    como_loop_handle_wrap,   2 },
    { "handle_unwrap",  como_loop_handle_unwrap, 1 },

    /* timer functions */
    { "timer_start", como_loop_timer_start, 3},
    { "timer_again", como_loop_timer_again, 1},
    { "update_time", como_loop_update_time, 1},

    /* io functions */
    { "io_start",  como_loop_io_start,      3 },
    { "io_stop",   como_loop_io_stop,       2 },
    { "io_remove", como_loop_io_remove,     2 },
    { "io_active", como_loop_io_active,     2 },
    
    
    { NULL, NULL, 0 }
};

static const duk_number_list_entry loop_constants[] = {
    { "POLLIN",  EV_POLLIN  },
    { "POLLOUT", EV_POLLOUT },
    { "POLLERR", EV_POLLERR },
    { NULL, 0 }
};

static int init_binding_loop(duk_context *ctx) {
    
    /* Initialize global stash 'comoHandles'. */
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "comoHandles");
    duk_pop(ctx);

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, loop_funcs);
    duk_put_number_list(ctx, -1, loop_constants);
    return 1;
}
