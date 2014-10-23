#include <stdio.h>

typedef struct {
    char *line;
    duk_context *ctx;
    void *self;
    int size;
    mtx_t gMutex;
} coThreadReadLine;

void _coReadLine (void *data) {
    coThreadReadLine *c = data;
    
    char buf[1024];
    char *res;
    size_t n = sizeof(buf);
    size_t len;
    
    while (1) {
        if ((res = fgets(buf, n, stdin)) != NULL) {
            mtx_lock(&c->gMutex);
            len = strlen(res);
            if (res[len-1] == '\n') {
                res[len-1] = '\0';
                c->size = len;
                c->line = malloc(sizeof(char) * len);
                strcpy(c->line, res);
            }
            mtx_unlock(&c->gMutex);
        }
    }
}

void _ReadLineHandlecb (evHandle *h) {
    coThreadReadLine *c = h->data;
    if (c->size > 0){
        mtx_lock(&c->gMutex);
        duk_context *ctx = c->ctx;
        void *self = c->self;
        duk_push_hobject(ctx, c->self);
        duk_get_prop_string(ctx, -1, "online");
        duk_push_hobject(ctx, c->self);
        duk_push_string(ctx, c->line);
        duk_call_method(ctx, 1);
        free(c->line);
        c->size = 0;
        mtx_unlock(&c->gMutex);
    }
}

static const int _readline_start(duk_context *ctx) {
    void *self = (void *)duk_require_hobject(ctx, 0);
    
    coThreadReadLine *c = malloc(sizeof(*c));
    memset(c, 0, sizeof(*c));
    c->ctx = ctx;
    c->self = self;
    c->line = NULL;
    c->size = 0;
    
    mtx_init(&c->gMutex, mtx_plain);
    
    thrd_t t;
    if (thrd_create(&t, (void *)_coReadLine, c) != thrd_success){
        assert(0 && "Can't Create Thread");
    }
    
    //FIXME: set a timer loop to check for new lines
    evLoop *loop = main_loop();
    evHandle *h = handle_init(loop, _ReadLineHandlecb);
    h->data = c;
    timer_start(h, 1, 1);
    
    duk_push_pointer(ctx, c);
    return 1;
}

static const duk_function_list_entry readline_funcs[] = {
    { "start"      , _readline_start, 1 },
    { NULL         , NULL, 0 }
};

static const int init_binding_readline(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, readline_funcs);
    return 1;
}
