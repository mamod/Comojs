#include <stdio.h>

typedef struct {
    char *line;
    duk_context *ctx;
    void *self;
    int size;
    mtx_t gMutex;
} comoThreadReadLine;

static void _comoReadLine (void *data) {
    comoThreadReadLine *c = data;
    
    char buf[1024];
    char *res;
    size_t n = sizeof(buf);
    size_t len;

    while (1) {
        if ((res = fgets(buf, n, stdin)) != NULL) {
            mtx_lock(&c->gMutex);
            len = strlen(res);
            if (len > 0){}

            if (res[len-1] == '\n') {
                res[len-1] = '\0';
                c->size = len;
                c->line = malloc(len);
                strcpy(c->line, res);
            }
            mtx_unlock(&c->gMutex);
        }
    }
}

static void _ReadLineHandlecb (evHandle *h) {
    comoThreadReadLine *c = h->data;
    if (c->size > 0){
        mtx_lock(&c->gMutex);
        duk_context *ctx = c->ctx;
        
        duk_push_heapptr(ctx, c->self);
        assert(duk_is_object(ctx, -1));
        
        duk_get_prop_string(ctx, -1, "online");
        duk_push_heapptr(ctx, c->self);
        duk_push_string(ctx, c->line);
        duk_call_method(ctx, 1);
        free(c->line);
        c->size = 0;
        duk_pop_n(ctx, 2);
        mtx_unlock(&c->gMutex);
    }
}

// static DWORD orig_consolemode = 0;
COMO_METHOD(como_readline_start) {
    void *self = duk_require_heapptr(ctx, 0);
    
    // DWORD n;
    // INPUT_RECORD irec;

    // HANDLE outh = GetStdHandle(STD_OUTPUT_HANDLE);
    // HANDLE inh = GetStdHandle(STD_INPUT_HANDLE);

    // if (!PeekConsoleInput(inh, &irec, 1, &n)) {
    //     return -1;
    // }
    // // if (getWindowSize(current) != 0) {
    // //     return -1;
    // // }
    // if (GetConsoleMode(inh, &orig_consolemode)) {
    //     SetConsoleMode(inh, ENABLE_PROCESSED_INPUT);
    // }

    comoThreadReadLine *c = malloc(sizeof(*c));
    memset(c, 0, sizeof(*c));
    c->ctx = ctx;
    c->self = self;
    c->line = NULL;
    c->size = 0;
    
    mtx_init(&c->gMutex, mtx_plain);
    
    thrd_t t;
    if (thrd_create(&t, (void *)_comoReadLine, c) != thrd_success ){
        assert(0 && "Can't Create Thread");
    }
    
    //FIXME: set a timer loop to check for new lines
    evLoop *loop = como_main_loop(ctx);
    evHandle *h  = handle_init(loop, _ReadLineHandlecb);
    h->data = c;
    timer_start(h, 1, 1);
    
    duk_push_pointer(ctx, c);
    return 1;
}

static const duk_function_list_entry readline_funcs[] = {
    { "start"        , como_readline_start, 1 },
    { NULL           , NULL, 0 }
};

static int init_binding_readline(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, readline_funcs);
    return 1;
}
