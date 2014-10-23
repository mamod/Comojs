#include "../loop/queue.h"

typedef struct {
    const char  *data_in;
    const char *data_out;
    int free;
    size_t id;
    void* queue[2];
} comoThreadQueue;

typedef struct {
    duk_context *ctx;
    duk_context *main_ctx;
    const char *worker;
    void *self;
    void *pending_queue[2];
    void *queue_in[2];
    void *queue_out[2];
    mtx_t gMutex;
} comoThread;

void _ThreadHandleConsume (evHandle *h) {
    comoThread *c = h->data;
    duk_context *ctx = c->ctx;
    void *self = c->self;
    QUEUE *q;
    comoThreadQueue *w;
    
    mtx_lock(&c->gMutex);
    if ( !QUEUE_EMPTY(&c->pending_queue) ) {
        q = QUEUE_HEAD(&c->pending_queue);
        QUEUE_REMOVE(q);
        QUEUE_INSERT_TAIL(&c->queue_in, q);
    }
    mtx_unlock(&c->gMutex);
    
    while ( !QUEUE_EMPTY(&c->queue_in) ) {
        q = QUEUE_HEAD(&c->queue_in);
        QUEUE_REMOVE(q);
        w = QUEUE_DATA(q, comoThreadQueue, queue);
        assert(w);
        
        if (w->free == 1){
            mtx_lock(&c->gMutex);
            free(w);
            mtx_unlock(&c->gMutex);
            continue;
        }
        
        duk_push_global_object(ctx);
        duk_get_prop_string(ctx, -1, "process");
        duk_get_prop_string(ctx, -1, "processthreadData");
        duk_push_string(ctx, w->data_in);
        duk_call(ctx, 1);
        const char *ret = duk_require_string(ctx, -1);
        w->data_in = ret;
        mtx_lock(&c->gMutex);
        QUEUE_INSERT_TAIL(&c->queue_out, q);
        mtx_unlock(&c->gMutex);
    }
}

int _como_WorkerThread (void *data) {
    comoThread *c = data;
    duk_context *ctx = c->ctx;
    duk_context *main = c->main_ctx;
    QUEUE *q;
    comoThreadQueue *w;
    
    create_new_thread_heap(ctx, c->worker);
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "process");
    duk_get_prop_string(ctx, -1, "processLoopid");
    evLoop *loop = duk_require_pointer(ctx, -1);
    evHandle *h = handle_init(loop, _ThreadHandleConsume);
    h->data = c;
    timer_start(h, 1, 1);
    loop_start(loop, 0);
    printf("done\n");
    return 0;
}

int _como_WorkerThreadxx (void *data) {
    comoThread *c = data;
    duk_context *ctx = c->ctx;
    duk_context *main = c->main_ctx;
    QUEUE *q;
    comoThreadQueue *w;
    
    create_new_thread_heap(ctx, c->worker);
    
    while (1){
        
        mtx_lock(&c->gMutex);
        while ( !QUEUE_EMPTY(&c->pending_queue) ) {
            q = QUEUE_HEAD(&c->pending_queue);
            QUEUE_REMOVE(q);
            QUEUE_INSERT_TAIL(&c->queue_in, q);
        }
        mtx_unlock(&c->gMutex);
        
        while ( !QUEUE_EMPTY(&c->queue_in) ) {
            q = QUEUE_HEAD(&c->queue_in);
            QUEUE_REMOVE(q);
            w = QUEUE_DATA(q, comoThreadQueue, queue);
            assert(w);
            
            if (w->free == 1){
                mtx_lock(&c->gMutex);
                free(w);
                mtx_unlock(&c->gMutex);
                continue;
            }
            
            duk_push_global_object(ctx);
            duk_get_prop_string(ctx, -1, "process");
            duk_get_prop_string(ctx, -1, "processthreadData");
            duk_push_string(ctx, w->data_in);
            duk_call(ctx, 1);
            
            const char *ret = duk_require_string(ctx, -1);
            duk_idx_t index = duk_get_top(ctx);
            duk_pop_n(ctx, index);
            w->data_in = ret;
            
            mtx_lock(&c->gMutex);
            QUEUE_INSERT_TAIL(&c->queue_out, q);
            mtx_unlock(&c->gMutex);
        }
        
        //give other threads some chance
        como_platform_sleep(1);
    }
    
    printf("done\n");
    return 0;
}

void _ThreadHandlecb (evHandle *h) {
    comoThread *c = h->data;
    duk_context *ctx = c->main_ctx;
    duk_context *mctx = c->ctx;
    void *self = c->self;
    QUEUE *q;
    comoThreadQueue *w;
    
    mtx_lock(&c->gMutex);
    while ( !QUEUE_EMPTY(&c->queue_out) ) {
        q = QUEUE_HEAD(&c->queue_out);
        
        w = QUEUE_DATA(q, comoThreadQueue, queue);
        QUEUE_REMOVE(q);
        
        duk_push_hobject(ctx, self);
        duk_get_prop_string(ctx, -1, "callback");
        duk_push_hobject(ctx, self);
        duk_push_string(ctx, w->data_in);
        duk_push_number(ctx, w->id);
        duk_call_method(ctx, 2);
        
        duk_idx_t index = duk_get_top(ctx);
        duk_pop_n(ctx, index);
        //dump_stack(ctx, "CTX");
        
        w->data_in = NULL;
        w->data_out = NULL;
        /*
         * Mark to be freed add it again to the pending queue
         * this time pending queue work will add to queue_in which will free
         * the comoThreadQueue struct
        */
        w->free = 1;
        QUEUE_INSERT_TAIL(&c->pending_queue, q);
    }
    mtx_unlock(&c->gMutex);
}

static const int _thread_create(duk_context *ctx) {
    const char *worker = duk_require_string(ctx, 0);
    void *self = (void *)duk_require_hobject(ctx, 1);
    
    //create new context
    duk_context *new_ctx = duk_create_heap(NULL, NULL, NULL, NULL, NULL);
    
    comoThread *c = malloc(sizeof(*c));
    if (c == NULL){
        COMO_FATAL_ERROR("ENOMEM : out of memory");
    }
    
    c->main_ctx = ctx;
    c->ctx = new_ctx;
    c->self = self;
    c->worker = worker;
    QUEUE_INIT(&c->queue_in);
    QUEUE_INIT(&c->queue_out);
    QUEUE_INIT(&c->pending_queue);
    mtx_init(&c->gMutex, mtx_plain);
    
    thrd_t t;
    if (thrd_create(&t, _como_WorkerThread, (void *)c) != thrd_success){
        COMO_FATAL_ERROR("Can't Create New Thread");
    }
    
    //set a timer watcher
    evLoop *loop = main_loop();
    evHandle *h = handle_init(loop, _ThreadHandlecb);
    h->data = c;
    timer_start(h, 1, 1);
    
    duk_push_pointer(ctx, c);
    return 1;
}

static const int _thread_send(duk_context *ctx) {
    comoThread *worker = duk_require_pointer(ctx, 0);
    const char *data = duk_require_string(ctx, 1);
    size_t id = (size_t)duk_require_int(ctx, 2);
    comoThreadQueue *wq = malloc(sizeof(*wq));
    wq->id = id;
    wq->data_in = data;
    wq->free = 0;
    mtx_lock(&worker->gMutex);
    QUEUE_INSERT_TAIL(&worker->pending_queue, &wq->queue);
    mtx_unlock(&worker->gMutex);
    return 1;
}

static const duk_function_list_entry thread_funcs[] = {
    { "create"      , _thread_create, 2 },
    { "send"        , _thread_send, 3 },
    { NULL          , NULL, 0 }
};

static const int init_binding_thread(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, thread_funcs);
    return 1;
}
