#include "../loop/queue.h"

mtx_t gCoro_mtx;

double g_coroid = 0;
void *coroQueue[2];

typedef struct comoCoro {
    duk_context *ctx;
    double id;
    void *self;
    void *queue[2];
} comoCoro;

comoCoro *current_coro;

void _como_free_coro (duk_context *ctx, comoCoro *coro){
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoCoros");
    duk_push_number(ctx, (double) coro->id);
    duk_del_prop(ctx, -2);
    duk_pop_n(ctx, 2);

    free(coro);
}

COMO_METHOD(como_coro_async) {
    void *self = duk_require_heapptr(ctx, 0);

    g_coroid++;
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoCoros");
    duk_push_number(ctx, g_coroid);
    duk_push_heapptr(ctx, self);
    duk_put_prop(ctx, -3); /* comoCoros[id] = callback */
    duk_pop_n(ctx, 2);

    comoCoro *coro = malloc(sizeof(*coro));
    coro->ctx = ctx;
    coro->self = self;
    coro->id = g_coroid;

    QUEUE_INIT(&coro->queue);
    QUEUE_INSERT_TAIL(&coroQueue, &coro->queue);
    duk_push_pointer(ctx, coro);
    return 1;
}

COMO_METHOD(como_coro_start) {
    
    QUEUE *q;
    if ( !QUEUE_EMPTY(&coroQueue) ){
        q = QUEUE_HEAD(&coroQueue);
        QUEUE_REMOVE(q);

        comoCoro *coro = QUEUE_DATA(q, comoCoro, queue);


        duk_context *ctx = coro->ctx;
        void *self = coro->self;

        duk_push_heapptr(ctx, self);
        duk_push_pointer(ctx, coro);
        duk_call(ctx, 1);
        dump_stack(ctx, "RET");
        duk_pop(ctx);
        // QUEUE_INSERT_TAIL(&coroQueue, &coro->queue);
    }
    return 1;
}

COMO_METHOD(como_coro_cede) {
    comoCoro *current = duk_require_pointer(ctx, 0);
    QUEUE_REMOVE(&current->queue);
    // QUEUE_INSERT_TAIL(&coroQueue, &current->queue);
    dump_stack(ctx, "RET");
    return 1;
}

COMO_METHOD(como_coro_wait) {
    
    if (duk_get_type(ctx, 0) == DUK_TYPE_POINTER){
        comoCoro *coro = duk_require_pointer(ctx, 0);
        QUEUE_REMOVE(&coro->queue);
        while (1){
            como_sleep(1);
            void *self = coro->self;
            duk_push_heapptr(ctx, self);
            duk_push_pointer(ctx, coro);
            duk_call(ctx, 1);
            int type = duk_get_type(ctx, -1);
            if (type != DUK_TYPE_NULL || type != DUK_TYPE_UNDEFINED){
                _como_free_coro(ctx, coro);
                return 1;
            }
        }
    } else {
        QUEUE *q;
        while ( !QUEUE_EMPTY(&coroQueue) ){
            q = QUEUE_HEAD(&coroQueue);
            QUEUE_REMOVE(q);

            comoCoro *coro = QUEUE_DATA(q, comoCoro, queue);

            duk_context *ctx = coro->ctx;
            void *self = coro->self;

            duk_push_heapptr(ctx, self);
            duk_push_pointer(ctx, coro);
            duk_call(ctx, 1);
            int type = duk_get_type(ctx, -1);
            if (type == DUK_TYPE_NULL || type == DUK_TYPE_UNDEFINED){
                QUEUE_INSERT_TAIL(&coroQueue, &coro->queue);
            } else {
                _como_free_coro(ctx, coro);
            }
            duk_pop(ctx);
        }
    }

    return 1;
}

static const duk_function_list_entry como_coro_funcs[] = {
    { "async"           , como_coro_async,        1 },
    { "start"           , como_coro_start,       0 },
    { "wait"            , como_coro_wait,       1 },
    { "cede"            , como_coro_cede,        1 },
    { NULL, NULL, 0 }
};

static int init_binding_coro(duk_context *ctx) {
    
    /* Initialize global stash 'comoCoros'. */
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "comoCoros");
    duk_pop(ctx);

    QUEUE_INIT(&coroQueue);

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_coro_funcs);
    return 1;
}
