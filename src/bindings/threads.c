
mtx_t como_gMutex; /* Como global mutex */
cnd_t como_gCond;  /* Como global Condition variable */
int como_initiated_mutex = 0;

typedef struct {
    mtx_t mtx;
    thrd_t t;
    char *bytecode;
    size_t bytecode_size;
    duk_context *Mainctx; /* main context */
    duk_context *ctx;
} comoThread;

COMO_METHOD(como_thread_join) {
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""threadP");
    comoThread *thread = duk_require_pointer(ctx, -1);
    duk_pop_2(ctx);

    thrd_join(thread->t, NULL);

    return 1;
}

static const duk_function_list_entry como_thread_init_func[] = {
    { "join",        como_thread_join ,         1 },
    { NULL,          NULL,                      0 }
};

int _como_child_thread (void *data){
    comoThread *thread = (comoThread *)data;

    if (thread->ctx == NULL){
        char *argv[0];
        int argc = 0;

        duk_context *ctx = como_create_new_heap (argc, argv, NULL);
        thread->ctx = ctx;

        size_t bytecode_size = thread->bytecode_size;
        char *buf = duk_push_fixed_buffer(ctx, bytecode_size);
        char *bytecode = thread->bytecode;

        int i = 0;
        for (i = 0; i < bytecode_size; i++){
            buf[i] = bytecode[i];
        }

        free(thread->bytecode);
        
        duk_load_function(ctx);
        
        //push this object
        duk_push_object(ctx);
        duk_put_function_list(ctx, -1, como_thread_init_func);
        duk_push_pointer(ctx, thread);
        duk_put_prop_string(ctx, -2, "\xff""threadP");

        duk_pcall_method(ctx, 0);
        duk_pop(ctx);
    }

    mtx_destroy(&thread->mtx);
    thrd_detach(thread->t);

    //force object finalizers
    duk_gc(thread->ctx, 0);
    duk_gc(thread->ctx, 0);

    duk_destroy_heap(thread->ctx);
    free(thread);
    return 0;
}

COMO_METHOD(como_threads_create) {
    duk_dump_function(ctx);
    size_t out_size;
    const char *func_bytecode = duk_require_buffer(ctx, 0, &out_size);

    comoThread *thread = malloc(sizeof(*thread));
    memset(thread, 0, sizeof(*thread));

    thread->Mainctx = ctx;
    thread->ctx = NULL;

    //make a copy of bytecode dump as we will use it later
    thread->bytecode = malloc(out_size);
    int i = 0;
    for (i = 0; i < out_size; i++){
        thread->bytecode[i] = func_bytecode[i];
    }

    thread->bytecode_size = out_size;
    mtx_init(&thread->mtx, mtx_plain);
    
    if (thrd_create(&thread->t, _como_child_thread, (void *)thread) != thrd_success){
        COMO_FATAL_ERROR("Can't Create New Thread");
    }

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_thread_init_func);
    duk_push_pointer(ctx, thread);
    duk_put_prop_string(ctx, -2, "\xff""threadP");

    return 1;
}

COMO_METHOD(como_threads_lock) {
    if ( mtx_lock(&como_gMutex) == thrd_success){
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }

    return 1;
}

COMO_METHOD(como_threads_trylock) {
    if ( mtx_trylock(&como_gMutex) == thrd_success){
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }

    return 1;
}

COMO_METHOD(como_threads_unlock) {
    if ( mtx_unlock(&como_gMutex) == thrd_success){
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }
    
    return 1;
}

static const duk_function_list_entry como_threads_funcs[] = {
    { "create",           como_threads_create,       1 },
    { "lock",             como_threads_lock,         1 },
    { "trylock",          como_threads_trylock,      1 },
    { "unlock",           como_threads_unlock,       1 },
    { NULL,               NULL,                      0 }
};

static int init_binding_threads(duk_context *ctx) {

    if (como_initiated_mutex == 0){
        mtx_init(&como_gMutex, mtx_plain);
        cnd_init(&como_gCond);
        como_initiated_mutex = 1;
    }

    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "comoThreadsCallBack");
    duk_pop(ctx);

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_threads_funcs);
    return 1;
}
