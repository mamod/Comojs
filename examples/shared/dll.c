#define BUILDING_NODE_EXTENSION
#include "../../src/como.h"
#include <stdio.h>

static const int _my_test(duk_context *ctx) {
    printf("called\n");
    duk_push_int(ctx, 888);
    return 1;
}

static const duk_function_list_entry my_funcs[] = {
    { "my_test", _my_test, 0 },
    { NULL, NULL, 0 }
};

COMO_MODULE_INIT() {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, my_funcs);
}
