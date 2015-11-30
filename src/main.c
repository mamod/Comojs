/** 
  * file : main.c
  * @author : Mamod Mehyar
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "main.h"
#include "como.h"

extern char **environ;

/* ALL Bindings ar directly included from main.h */
static const duk_function_list_entry bindings_funcs[] = {
    { "threads"     , init_binding_threads,   0 },
    { "worker"      , init_binding_worker,    0 },
    { "buffer"      , init_binding_buffer,    0 },
    { "loop"        , init_binding_loop,      0 },
    { "socket"      , init_binding_socket,    0 },
    { "http-parser" , init_binding_parser,    0 },
    { "errno"       , init_binding_errno,     0 },
    { "sys"         , init_binding_sys,       0 },
    { "io"          , init_binding_io,        0 },
    { "readline"    , init_binding_readline,  0 },
    { "tty"         , init_binding_tty,       0 },
    { "fs"          , init_binding_fs,        0 },
    { "tls"         , init_binding_tls,       0 },
    { "crypto"      , init_binding_crypto,    0 },
    { "posix"       , init_binding_posix,     0 },
    { NULL          , NULL, 0 }
};

/** 
  * quick substring function for parsing ENV
******************************************************************************/
static char *_como_substring(char *string, int position, int length) {
    int c;
    char *pointer = malloc(length+1);
    if (pointer == NULL) {
        fprintf(stderr, "Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    
    for (c = 0 ; c < position -1 ; c++) string++;
    for (c = 0 ; c < length ; c++) {
        *(pointer+c) = *string;
        string++;
    }
    
    *(pointer+c) = '\0';
    return pointer;
}

/** 
  * parse environ
******************************************************************************/
static void _como_parse_environment (duk_context *ctx, duk_idx_t obj_idx){
    int i = 1;
    char *s = *environ;
    duk_idx_t sub_obj_idx = duk_push_object(ctx);
    
    for (; s; i++) {
        char *ENV_name;
        char *ENV_value;
        int index;
        ENV_value = strchr(s, '=');
        if (ENV_value != NULL) {
            index = (int)(ENV_value - s);
            ENV_name = _como_substring(s, 0, index);
            
            /* pass equal sign */
            ENV_value++;
            
            duk_push_string(ctx, ENV_value);
            duk_put_prop_string(ctx, sub_obj_idx, ENV_name);
            free(ENV_name);
        }
        
        s = *(environ+i);
    }
    
    duk_put_prop_string(ctx, obj_idx, "env");
}

/** 
  * process.isFile(num)
******************************************************************************/
COMO_METHOD(como_process_isFile) {
    const char *filename = duk_get_string(ctx, 0);
    struct stat buffer;
    if (stat (filename, &buffer) == 0) {
        duk_push_true(ctx);
    } else {
        duk_push_false(ctx);
    }
    return 1;
}

/** 
  * process.readFile(filename);
******************************************************************************/
COMO_METHOD(como_process_readFile) {
    const char *filename = duk_get_string(ctx, 0);
    FILE *f = NULL;
    size_t len;
    void *buf;
    size_t got;
    if (!filename) {
        goto error;
    }
    
    f = fopen(filename, "rb");
    if (!f) goto error;
    
    if (fseek(f, 0, SEEK_END) != 0) goto error;
    
    len = ftell(f);
    if (fseek(f, 0, SEEK_SET) != 0) goto error;
    
    buf = duk_push_fixed_buffer(ctx, len);
    got = fread(buf, 1, len, f);
    if (got != len) goto error;
    
    fclose(f);
    f = NULL;
    return 1;
    
    error:
        if (f) {
            fclose(f);
        }
    
    return DUK_RET_ERROR;
}

/** 
  * process.sleep(num);
******************************************************************************/
COMO_METHOD(como_process_sleep) {
    int milliseconds = duk_get_int(ctx, 0);
    duk_pop(ctx);
    #ifdef _WIN32
        Sleep(milliseconds);
    #else
        usleep(1000 * milliseconds);
    #endif
    return 1;
}

/** 
  * process.cwd();
******************************************************************************/
COMO_METHOD(como_process_cwd) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        duk_push_string(ctx, cwd);
    } else {
        duk_push_string(ctx, "");
    }
    return 1;
}

/** 
  * process.eval(str);
******************************************************************************/
COMO_METHOD(como_process_eval) {
    const char *buf = duk_get_string(ctx, 0);
    const char *filename = duk_get_string(ctx, 1);
    duk_push_string(ctx, filename);
    if (duk_pcompile_string_filename(ctx, 0, buf)){
        printf("%s\n", duk_safe_to_string(ctx, -1));
        printf("%s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }
    duk_call(ctx, 0);
    return 1;
}

/** 
  * process.exit(exit_status);
******************************************************************************/
COMO_METHOD(como_process_exit) {
    int exitCode = duk_get_int(ctx, 0);
    duk_destroy_heap(ctx);
    exit(exitCode);
    return 1;
}

/** 
  * process.dlOpen(shared_library);
******************************************************************************/
COMO_METHOD(como_process_dllOpen) {
    const char *ModuleName = duk_get_string(ctx, 0);

    void * handle = dlopen(ModuleName, RTLD_LAZY);
    if (!handle){
        printf("Loading Module %s Aborted\n", ModuleName);
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    typedef void (*init_t)(duk_context *ctx, const char *ModuleName);
    init_t func = (init_t) dlsym(handle, "_como_init_module");

    if (!func){
        printf("Loading Module %s Aborted\n", ModuleName);
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    //register require for this module in global stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "modules");
    duk_push_string(ctx, ModuleName);
    duk_dup(ctx, 1);
    duk_put_prop(ctx, -3);

    //init module
    func(ctx, ModuleName);
    return 1;
}

/** 
  * process exported functions list
******************************************************************************/
const duk_function_list_entry process_funcs[] = {
    { "isFile"  , como_process_isFile, 1 },
    { "readFile", como_process_readFile, 1 },
    { "cwd"     , como_process_cwd, 0 },
    { "eval"    , como_process_eval, 2 },
    { "sleep"   , como_process_sleep, 1 },
    { "dllOpen" , como_process_dllOpen, 2 },
    { "exit"    , como_process_exit, 1 },
    { NULL, NULL, 0 }
};

void como_init_process(int argc, char *argv[], duk_context *ctx) {
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, process_funcs);
    
    /* process ENV */
    _como_parse_environment(ctx, -2);
    
    /* process id*/
    duk_push_int(ctx, getpid());
    duk_put_prop_string(ctx, -2, "pid");
    
    /* process platform*/
    duk_push_string(ctx, PLATFORM);
    duk_put_prop_string(ctx, -2, "platform");
    
    /* argv */
    duk_idx_t arr_idx = duk_push_array(ctx);
    int i = 0;
    for (i = 0; i < argc; i++){
        duk_push_string(ctx, argv[i]);
        duk_put_prop_index(ctx, arr_idx, i);
    }
    duk_put_prop_string(ctx, -2, "argv");
    
    /* bindings */
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, bindings_funcs);
    duk_put_prop_string(ctx, -2, "bindings");
    
    duk_put_prop_string(ctx, -2, "process");
    duk_pop(ctx);
    
    /* Initialize global stash 'modules'. */
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, "modules");
    duk_pop(ctx);
}

duk_context *como_create_new_heap (int argc, char *argv[], void *error_handle) {
    duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, error_handle);
    como_init_process(argc, argv, ctx);
    return ctx;
}

void como_run (duk_context *ctx){
    if (duk_peval_file(ctx, "js/main.js") != 0) {
        printf("%s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }
}

#ifndef COMO_SHARED
int main(int argc, char *argv[], char** envp) {
    (void) argc; (void) argv;
    
    duk_context *ctx = como_create_new_heap(argc, argv, NULL);
    como_run(ctx);
    duk_pop(ctx);  /* pop eval result */

    //force object finalizers
    duk_gc(ctx, 0);
    duk_gc(ctx, 0);

    duk_destroy_heap(ctx);
    return 0;
}
#endif
