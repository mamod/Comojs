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
    { "buffer"      , init_binding_buffer,   0 },
    { "loop"        , init_binding_loop,     0 },
    { "socket"      , init_binding_socket,   0 },
    { "http-parser" , init_binding_parser,   0 },
    { "errno"       , init_binding_errno,    0 },
    { "thread"      , init_binding_thread,   0 },
    { "io"          , init_binding_io,       0 },
    { "readline"    , init_binding_readline, 0 },
    { "test"          , init_binding_test,   0 },
    { NULL          , NULL, 0 }
};

static void debug_top(duk_context *ctx){
    printf("stack top is %ld\n", (long) duk_get_top(ctx));
}

/** 
  * quick substring function for parsing ENV
******************************************************************************/
static char *substring(char *string, int position, int length) {
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
            ENV_name = substring(s, 0, index);
            
            /* pass equal sign */
            *ENV_value++;
            
            duk_push_string(ctx, ENV_value);
            duk_put_prop_string(ctx, sub_obj_idx, ENV_name);
            free(ENV_name);
            ENV_name = NULL;
        }
        
        s = *(environ+i);
    }
    
    duk_put_prop_string(ctx, obj_idx, "env");
}

/** 
  * process.isFile(num)
******************************************************************************/
static const int process_isFile(duk_context *ctx) {
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
const int process_readFile(duk_context *ctx) {
    const char *filename = duk_to_string(ctx, 0);
    FILE *f = NULL;
    long len;
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
    
    buf = duk_push_fixed_buffer(ctx, (size_t) len);
    got = fread(buf, 1, len, f);
    if (got != (size_t) len) goto error;
    
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
static const int process_sleep(duk_context *ctx) {
    int milliseconds = duk_get_int(ctx, 0);
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
static const int process_cwd(duk_context *ctx) {
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
static const int process_eval(duk_context *ctx) {
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
static const int process_exit(duk_context *ctx) {
    int exitCode = duk_get_int(ctx, 0);
    duk_destroy_heap(ctx);
    exit(exitCode);
    return 1;
}

/** 
  * process.dlOpen(shared_library);
******************************************************************************/
static const int process_dllOpen(duk_context *ctx) {
    const char *filename = duk_get_string(ctx, 0);
    void * handle = dlopen(filename, RTLD_LAZY);
    if (!handle){
        printf("ERROR\n");
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    typedef void (*init_t)(duk_context *ctx);
    init_t func = (init_t) dlsym(handle, "init");

    if (!func){
        printf("ERROR\n");
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
    func(ctx);
    return 1;
}

/** 
  * process exported functions list
******************************************************************************/
const duk_function_list_entry process_funcs[] = {
    { "isFile"  , process_isFile, 1 },
    { "readFile", process_readFile, 1 },
    { "cwd"     , process_cwd, 0 },
    { "eval"    , process_eval, 2 },
    { "sleep"   , process_sleep, 1 },
    { "dllOpen"    , process_dllOpen, 1 },
    { "exit"    , process_exit, 1 },
    { NULL, NULL, 0 }
};

void create_new_thread_heap (duk_context *ctx, const char *worker) {
    
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, process_funcs);
    
    /* process ENV */
    _como_parse_environment(ctx, -2);
    
    /* argv */
    duk_idx_t arr_idx = duk_push_array(ctx);
    duk_push_string(ctx, "--thread");
    duk_put_prop_index(ctx, arr_idx, 0);
    
    duk_push_string(ctx, worker);
    duk_put_prop_index(ctx, arr_idx, 1);
    duk_put_prop_string(ctx, -2, "argv");
    
    /* process id*/
    duk_push_int(ctx, getpid());
    duk_put_prop_string(ctx, -2, "pid");
    
    /* process platform*/
    duk_push_string(ctx, PLATFORM);
    duk_put_prop_string(ctx, -2, "platform");
    
    /* bindings */
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, bindings_funcs);
    duk_put_prop_string(ctx, -2, "bindings");
    
    duk_put_prop_string(ctx, -2, "process");
    duk_pop(ctx);
    
    //dump_global_object_keys(ctx);
    if (duk_peval_file(ctx, "lib/main.js") != 0) {
        printf("%s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }
    
    duk_pop(ctx);  /* pop eval result */
}

int main(int argc, char *argv[], char** envp) {

    (void) argc; (void) argv;
    
    duk_context *ctx = duk_create_heap(NULL,NULL,NULL,NULL,NULL);

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
    for (i; i < argc; i++){
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
    
    if (duk_peval_file(ctx, "lib/main.js") != 0) {
        printf("%s\n", duk_safe_to_string(ctx, -2));
        duk_destroy_heap(ctx);
        exit(1);
    }
    duk_pop(ctx);  /* pop eval result */
    
    //debug_top(ctx);
    duk_destroy_heap(ctx);
    return 0;
}
