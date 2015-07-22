#include "../loop/queue.h"

typedef struct {
    mtx_t mtx;
    thrd_t t;
    int destroy;
    duk_context *Mainctx; /* main context */
    duk_context *ctx;
    char *file;
    void *self;
    void *queueIn[2];
    void *queueOut[2];
} comoWorker;

typedef struct {
    void *data;
    duk_int_t type; /* data type */
    void *queue[2];
} comoQueue;

void como_push_worker_value (duk_context *ctx, comoQueue *queue){
    switch(queue->type){
        case DUK_TYPE_UNDEFINED :
            duk_push_undefined(ctx);
            break;

        case DUK_TYPE_NULL :
                duk_push_null(ctx);
                break;

        case DUK_TYPE_OBJECT :
            duk_push_string(ctx, queue->data);
            duk_json_decode(ctx, -1);
            break;

        case DUK_TYPE_POINTER :
            duk_push_pointer(ctx, queue->data);
            break;

        case DUK_TYPE_STRING :
            duk_push_string(ctx, queue->data);
            break;
        
        default :
            assert(0 && "Duk Type Error");
    }
}

int _como_worker_thread (void *data){
    comoWorker *worker = data;
    QUEUE *q;

    if (worker->ctx == NULL){
        char *argv[3];
        int argc = 3;
        const char *arg = "";
        const char *prefix = "--childWorker";

        argv[0] = (char *)arg;
        argv[1] = worker->file;
        argv[2] = (char *)prefix;

        duk_context *ctx = como_create_new_heap (argc, argv, NULL);
        worker->ctx = ctx;
        como_run(ctx);
    }

    while (1){
        
        como_sleep(1);
        while ( !QUEUE_EMPTY(&worker->queueIn) ){
            mtx_lock(&worker->mtx);
            q = QUEUE_HEAD(&(worker)->queueIn);
            QUEUE_REMOVE(q);
            comoQueue *queue = QUEUE_DATA(q, comoQueue, queue);
            mtx_unlock(&worker->mtx);

            if (worker->destroy != 0){
                goto FREE;
            }

            como_push_worker_value(worker->ctx, queue);

            duk_push_pointer(worker->ctx, worker);
            duk_call(worker->ctx, 2);
            
            FREE :
            if (queue->data != NULL && queue->type != DUK_TYPE_POINTER){
                free(queue->data);
            }

            free(queue);
        }
        
        //call this to run event loop only
        duk_call(worker->ctx, 0);
        
        if (worker->destroy == 1){
            worker->destroy = 2; /* pass destruction to main thread */
            
            duk_push_global_object(worker->ctx);
            duk_get_prop_string(worker->ctx, -1, "process");
            duk_get_prop_string(worker->ctx, -1, "_emitExit");
            // dump_stack(worker->ctx, "PP");
            duk_call(worker->ctx, 0);
            break;
        }
    }

    duk_destroy_heap(worker->ctx);
    return 0;
}

static int _worker_dispatch_cb (evHandle *handle){
    comoWorker *worker = handle->data;
    duk_context *ctx = worker->Mainctx;

    mtx_lock(&worker->mtx);
    QUEUE *q;
    while ( !QUEUE_EMPTY(&worker->queueOut) ){

        q = QUEUE_HEAD(&(worker)->queueOut);
        QUEUE_REMOVE(q);
        comoQueue *queue = QUEUE_DATA(q, comoQueue, queue);

        if (worker->destroy != 0){
            goto FREE;
        }

        duk_push_heapptr(ctx, worker->self);
        
        if (duk_get_type(ctx, -1) != DUK_TYPE_OBJECT){
            dump_stack(ctx, "DUK");
            assert(0);
        }

        como_push_worker_value(ctx, queue);

        duk_call(ctx, 1);
        duk_pop(ctx);

        FREE :
        /* free except in case of pointers */
        if (queue->data != NULL && queue->type != DUK_TYPE_POINTER){
            free(queue->data);
        }

        free(queue);
    }
    mtx_unlock(&worker->mtx);

    if (worker->destroy == 2){
        
        duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, "comoWorkersCallBack");
        duk_push_number(ctx, (double) handle->id);
        duk_del_prop(ctx, -2);

        handle_close(handle);
        free(worker);
    }

    return 0;
}

COMO_METHOD(como_worker_init) {

    evLoop *loop     = duk_require_pointer(ctx, 0);
    const char *file = duk_require_string(ctx, 1);
    void *self       = duk_require_heapptr(ctx, 2);
    

    comoWorker *worker = malloc(sizeof(*worker));
    memset(worker, 0, sizeof(*worker));
    QUEUE_INIT(&worker->queueIn);
    QUEUE_INIT(&worker->queueOut);

    worker->Mainctx = ctx;
    worker->self = self;
    worker->file = (char *)file;
    worker->destroy = 0;

    evHandle *handle  = handle_init(loop, _worker_dispatch_cb);
    handle->data = worker;
    
    /* save callback reference in global stash */
    int64_t handle_id = handle->id;
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "comoWorkersCallBack");
    duk_push_number(ctx, (double) handle_id);
    duk_dup(ctx, 2);
    duk_put_prop(ctx, -3); /* comoHandles[handle_id] = callback */

    mtx_init(&worker->mtx, mtx_plain);
    
    if (thrd_create(&worker->t, _como_worker_thread, (void *)worker) != thrd_success){
        COMO_FATAL_ERROR("Can't Create New Thread");
    }
    
    timer_start(handle, 1, 1);
    duk_push_pointer(ctx, worker);
    return 1;
}

comoQueue *como_duk_val_to_queue (duk_context *ctx, duk_idx_t index) {

    const char *string = NULL;

    comoQueue *queue = malloc(sizeof(*queue));
    memset(queue, 0, sizeof(*queue));
    QUEUE_INIT(&queue->queue);

    queue->type = duk_get_type(ctx, index);
    queue->data = NULL;

    switch(queue->type){
        case DUK_TYPE_UNDEFINED :
        case DUK_TYPE_NULL :
            break;

        case DUK_TYPE_NUMBER :
            COMO_FATAL_ERROR("NUMBERS SUPPORT IN PROGRESS");
            break;

        case DUK_TYPE_OBJECT :
            string = duk_json_encode(ctx, index);
            break;

        case DUK_TYPE_POINTER :
            queue->data = duk_get_pointer(ctx, index);
            break;

        case DUK_TYPE_STRING :
            string = duk_get_string(ctx, index);
            break;
    }

    if (string != NULL){
        queue->data = malloc( strlen(string) + 1);
        strcpy(queue->data, (char *)string);
    }

    return queue;
}

COMO_METHOD(como_post_message_to_child) {
    comoWorker *worker = duk_require_pointer(ctx, 0);
    comoQueue *queue   = como_duk_val_to_queue(ctx, 1);

    QUEUE_INSERT_TAIL(&worker->queueIn, 
                      &queue->queue);

    return 1;
}

COMO_METHOD(como_post_message_to_parent) {
    comoWorker *worker = duk_require_pointer(ctx, 0);
    comoQueue *queue   = como_duk_val_to_queue(ctx, 1);
    
    mtx_lock(&worker->mtx);
    QUEUE_INSERT_TAIL(&worker->queueOut, 
                      &queue->queue);
    mtx_unlock(&worker->mtx);
    return 1;
}

COMO_METHOD(como_destroy_worker) {
    comoWorker *worker = duk_require_pointer(ctx, 0);
    worker->destroy = duk_require_int(ctx, 1);
    return 1;
}

static const duk_function_list_entry como_worker_funcs[] = {
    { "init",               como_worker_init ,            3 },
    { "postMessageToChild",  como_post_message_to_child,  2 },
    { "postMessageToParent", como_post_message_to_parent, 2 },
    { "destroyWorker",       como_destroy_worker,         2 },
    { NULL,                 NULL,                         0 }
};

static int init_binding_worker(duk_context *ctx) {
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "comoWorkersCallBack");
    duk_pop(ctx);

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_worker_funcs);
    return 1;
}
