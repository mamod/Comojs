#ifndef _COMO_H
#define _COMO_H

#include "duktape/duktape.h"

const char *_COMO_MODULE_PATH = "";

#ifdef __cplusplus
    #define EXTERNC extern "C"
#else
    #define EXTERNC
#endif

#define MODULE_EXPORT(ctx, filename) EXTERNC void _como_module_export (duk_context *ctx, const char *filename)
#define _COMO_INIT void _como_init_module(duk_context *ctx, const char *filename)

#ifdef _WIN32
    #include <windows.h>
    #define dlsym(x, y) GetProcAddress((HMODULE)x, y)
    #define COMO_MODULE_INIT(ctx, filename) EXTERNC __declspec(dllexport) _COMO_INIT
#else
    #include <dlfcn.h>
    #define COMO_MODULE_INIT(ctx, filename) EXTERNC _COMO_INIT
#endif

void como_init_process(int argc, char *argv[], duk_context *ctx);

//FIXME : Error checking?
void como_require(duk_context *ctx, const char *file) {
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "modules");
    duk_get_prop_string(ctx, -1, _COMO_MODULE_PATH);
    duk_push_string(ctx, file);
    duk_call(ctx, 1);
}

void como_import(duk_context *ctx, const char *prop, const char *module) {
    como_require(ctx, module);
    duk_get_prop_string(ctx, -1, prop);
}

#ifndef _COMO_CORE_H
COMO_MODULE_INIT (ctx, filename) {
    _COMO_MODULE_PATH = filename;
    _como_module_export(ctx,filename);
}
#endif

#endif /*_COMO_H*/
