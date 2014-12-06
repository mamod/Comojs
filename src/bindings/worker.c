#include "../queue.h"

SIMPLEQ_HEAD(ComoInOut, comoWorkerJob);

struct comoWorkerJob {
    char *data;
    SIMPLEQ_ENTRY(comoWorkerJob) entries;
};

typedef struct {
    mtx_t mtx;
    cnd_t cond;
    int pool;
    int total;
    const char *file;
    const char *arg;
    evHandle *handle;
    struct ComoInOut in;
    struct ComoInOut out;
} comoThreadQueue;

typedef struct {
    mtx_t mtx;
    thrd_t t;
    duk_context *ctx;
    comoThreadQueue *queue;
} comoWorker;

/*=============================================================================
  function which will run in threads, it will execute new created context, 
  create new event loop and then pass everything to the javascript land
 ============================================================================*/
int _como_thread_start (void *data) {

    comoWorker *worker = data;
    comoThreadQueue *queue = worker->queue;
    duk_context *ctx = worker->ctx;
    
    /* stack => [function(){}] anon function from
    child thread just to save thread & queue pointers */
    como_run(ctx);

    duk_push_pointer(ctx, worker);
    duk_push_pointer(ctx, queue);

    /* this function will block in javascript land and will
    not return unless we are done with this thread */
    duk_call(ctx, 2);
    evLoop *loop = duk_require_pointer(ctx, 0);

    /* free this thread */

    int retval;
    thrd_join(&worker->t, &retval);
    
    mtx_lock(&queue->mtx);
    queue->total--;
    if (queue->total == 0){
        handle_unref(queue->handle);
    }
    mtx_unlock(&queue->mtx);

    free(loop->api);
    free(loop);
    free(worker);
    duk_destroy_heap(ctx);
    return 0;
}

/*=============================================================================
  initialize a queue for the worker, this queue will be shared with the pool
  of workers created
 ============================================================================*/
void como_worker_create(comoThreadQueue *queue) {
    
    const char *file       = queue->file;
    const char *arg        = queue->arg;

    char *argv[3];
    int argc = 3;

    argv[0] = (char *)arg;
    argv[1] = (char *)file;
    argv[2] = "--childWorker";
    
    comoWorker *worker = malloc(sizeof(*worker));
    memset(worker, 0, sizeof(*worker));
    worker->queue = queue;

    duk_context *ctx = como_create_new_heap (argc, argv);
    worker->ctx = ctx;

    /* lock => will be unlocked from thread
    just after everything in place */
    
    if (thrd_create(&worker->t, _como_thread_start, (void *)worker) != thrd_success){
        COMO_FATAL_ERROR("Can't Create New Thread");
    }

    como_sleep(100);
    return 1;
}

/*=============================================================================
  this thread is created for the sole purpose to manage other threads pool

  since duktape requires a seperate context for each thread, initializing new
  context with all globals and event loop is kinda expensive so we need to 
  create it within a thread to avoid main thread lagging
 ============================================================================*/
int _como_worker_manager (void *data) {

    comoThreadQueue *queue = data;
    
    while(1){
        mtx_lock(&queue->mtx);
        if (queue->pool > queue->total && !SIMPLEQ_EMPTY(&queue->in)){
            queue->total++;
            mtx_unlock(&queue->mtx);
            como_worker_create(queue);
            continue;
        } else {
            cnd_wait(&queue->cond, &queue->mtx);
        }
        mtx_unlock(&queue->mtx);
    }

    return 0;
}

/*=============================================================================
  initialize a queue for the worker, this queue will be shared with the pool
  of workers created
 ============================================================================*/
static const int como_worker_queue(duk_context *ctx) {

    const char *file       = duk_require_string(ctx, 0);
    const char *arg        = duk_require_string(ctx, 1);
    int pool               = duk_require_int(ctx, 2);
    evHandle *handle       = duk_require_pointer(ctx, 3);

    comoThreadQueue *queue = malloc(sizeof(*queue));
    memset(queue, 0, sizeof(*queue));

    SIMPLEQ_INIT(&queue->in);
    SIMPLEQ_INIT(&queue->out);
    
    cnd_init(&queue->cond);
    mtx_init(&queue->mtx, mtx_plain);

    queue->file  = file;
    queue->arg   = arg;
    queue->pool  = pool;
    queue->total = 0;
    queue->handle = handle;
    handle_unref(queue->handle);

    thrd_t t;
    if (thrd_create(&t, _como_worker_manager, (void *)queue) != thrd_success){
        COMO_FATAL_ERROR("Can't Create New Thread");
    }
    
    duk_push_pointer(ctx, queue);
    return 1;
}

/*=============================================================================
  push message from and to worker, will be used when calling 
  w.postMessage()

  this function will be called from both worker and main thread
 ============================================================================*/
static const int como_worker_post_message(duk_context *ctx) {

    size_t size, i;
    const char *data        = duk_require_lstring(ctx, 0, &size);
    comoThreadQueue *queue  = duk_require_pointer(ctx, 1);
    int isWorker            = duk_get_int(ctx, 2);

    struct comoWorkerJob *job = malloc(sizeof(*job));
    memset(job, 0, sizeof(*job));

    job->data = malloc(size+1);
    for (i = 0; i < size+1; i++){
        job->data[i] = data[i];
    }

    mtx_lock(&queue->mtx);
    if (isWorker){
        SIMPLEQ_INSERT_TAIL(&queue->out, job, entries);
    } else {
        handle_ref(queue->handle);
        SIMPLEQ_INSERT_TAIL(&queue->in, job, entries);
        cnd_broadcast(&queue->cond);
    }
    mtx_unlock(&queue->mtx);
    duk_pop_n(ctx, 3);
    return 1;
}

/*=============================================================================
  work submit and work done watcher, for both worker and main thread
 ============================================================================*/
static const int como_worker_watcher(duk_context *ctx) {
    
    comoThreadQueue *queue = duk_require_pointer(ctx, 0);
    int isWorker           = duk_get_int(ctx, 1);
    
    struct ComoInOut *inout;
    
    if (isWorker){
        inout = &queue->in;
    } else {
        inout = &queue->out;
    }

    mtx_lock(&queue->mtx);
    if (!SIMPLEQ_EMPTY(inout)){
        struct comoWorkerJob *job = SIMPLEQ_FIRST(inout);
        SIMPLEQ_REMOVE_HEAD(inout, entries);
        duk_push_string(ctx, job->data);
        free(job->data);
        free(job);
    } else {
        duk_push_null(ctx);
    }
    mtx_unlock(&queue->mtx);
    return 1;
}

static const duk_function_list_entry como_worker_funcs[] = {
    { "queue_init"   , como_worker_queue ,        4 },
    { "post_message" , como_worker_post_message,  3 },
    { "watcher"      , como_worker_watcher,       2 },
    { NULL           , NULL, 0 }
};

static const int init_binding_worker(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_worker_funcs);
    return 1;
}
