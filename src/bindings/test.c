
typedef struct fakeStruct {
    duk_context *ctx;
    void *self;
    void *cb;
} fakeStruct;

int (*functionPtr)();
fakeStruct *gstr;

static const int _test_save(duk_context *ctx) {
    void *p = duk_to_pointer(ctx, 0);
    gstr = malloc(sizeof(*gstr));
    gstr->cb = p;
    gstr->ctx = ctx;
    return 1;
}

static const int _test_restore(duk_context *ctx) {
    printf("called\n");
    duk_context *ctx2 = gstr->ctx;
    void *cb = gstr->cb;

    duk_push_global_object(ctx2);
    duk_push_hobject(ctx2, cb);
    dump_stack(ctx2, "CB");
    duk_call(ctx2,0);
    return 1;
}

static const duk_function_list_entry como_test_funcs[] = {
    { "save", _test_save,   3 },
    { "restore", _test_restore,   3 },
    { NULL, NULL, 0 }
};

static const int init_binding_test(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_test_funcs);
    // duk_put_number_list(ctx,   -1, como_io_constants);
    return 1;
}
