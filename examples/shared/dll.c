#include "../../src/como.h"
#include <stdio.h>

static const int _my_own_call(duk_context *ctx) {
    printf("called from dll\n");
    //requiring another js module
    //path resolving just mormal js require
    como_require(ctx, "./another/index");
    duk_call(ctx,0);
    duk_push_int(ctx, 1);
    return 1;
}

static const int _call_me(duk_context *ctx) {
    printf("called\n");

    //require from c
    como_require(ctx, "./dll.js");
    duk_get_prop_string(ctx, -1, "hi");
    duk_call(ctx,0);
    
    //we can also require another c module
    //or same module if that makes any point
    como_require(ctx, "./dll.como");
    duk_get_prop_string(ctx, -1, "call_again");
    duk_call(ctx,0);

    //or short cut
    como_import(ctx, "bye", "./dll.js");
    duk_call(ctx,0);

    duk_push_int(ctx, 1);
    return 1;
}

static const duk_function_list_entry my_funcs[] = {
    { "call_me", _call_me, 0 },
    { "call_again", _my_own_call, 0 },
    { NULL, NULL, 0 }
};

MODULE_EXPORT (ctx, filename) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, my_funcs);
}
