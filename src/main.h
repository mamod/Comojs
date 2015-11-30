#ifndef _COMO_CORE_H
#define _COMO_CORE_H

#include <errno.h>

#ifdef _WIN32
    #ifdef __TINYC__
        #include "tinycc/assert.h"
        #include "tinycc/winsock2.h"
        #include "tinycc/mswsock.h"
        #include "tinycc/ws2tcpip.h"
    #else
        #include <winsock2.h>
        #include <mswsock.h>
        #include <ws2tcpip.h>
        #include <assert.h>
    #endif
    
    #include <windows.h>
    
    #define dlopen(x,y) (void*)LoadLibrary(x)
    #define dlclose(x) FreeLibrary((HMODULE)x)
    #define COMO_GET_LAST_ERROR GetLastError()

    static inline int _WSLASTERROR (){
        int er = WSAGetLastError();
        if (er == WSAEINVAL){
            return EINVAL;
        }
        return er;
    }
    
    #define COMO_GET_LAST_WSA_ERROR _WSLASTERROR()
#else
    #include <assert.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <dlfcn.h>
    #define COMO_GET_LAST_ERROR errno

    /* win32 socket compatible constants */
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define COMO_GET_LAST_WSA_ERROR errno
#endif

#include "duktape/duktape.h"
#include "thread/tinycthread.h"

void dump_stack(duk_context *ctx, const char *name) {
    duk_idx_t i, n;
    n = duk_get_top(ctx);
    printf("%s (top=%ld):", name, (long) n);
    for (i = 0; i < n; i++) {
        printf(" ");
        duk_dup(ctx, i);
        printf("%s", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
    }
    printf("\n");
    fflush(stdout);
}

/* Need to define _wassert as tcc doesn't recognize it on win32 ?!!*/
#ifndef _wassert
#define _wassert assert
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define COMO_METHOD(name) static int (name)(duk_context *ctx)
#define COMO_METHOD_NAME(name)  name


#define COMO_SET_ERRNO(ctx, err) do \
{\
    errno = err; \
    duk_push_global_object(ctx); \
    duk_get_prop_string(ctx, -1, "process"); \
    duk_push_string(ctx, "errno"); \
    duk_push_int(ctx, err); \
    duk_put_prop(ctx, -3); \
} while(0)

#define COMO_SET_ERRNO_AND_RETURN(ctx, err) do \
{\
    COMO_SET_ERRNO(ctx,err);\
    duk_push_null(ctx);\
    return 1;\
} while(0)

#define COMO_FATAL_ERROR(err) do \
{                              \
    fprintf(stderr, "\n\n--> ");   \
    fprintf(stderr, err);      \
    fprintf(stderr, "\n\n");     \
    assert(0); \
} while(0)


void como_run (duk_context *ctx);
duk_context *como_create_new_heap (int argc, char *argv[], void *error_handle);

void como_sleep (int timeout){
    #ifdef _WIN32
        Sleep(timeout);
    #else
        usleep(1000 * timeout);
    #endif
}

/* BINDINGS */

#include "utils/inet.c"
#include "utils/socketpair.c"

#ifndef _WIN32
#include "utils/core.c"
#endif

#include "bindings/errno.c"
#include "bindings/sys.c"
#include "bindings/posix.c"
#include "bindings/loop.c"
#include "bindings/buffer.c"
#include "bindings/socket.c"
#include "bindings/io.c"
#include "bindings/http-parser.c"
#include "bindings/readline.c"
#include "bindings/worker.c"
#include "bindings/tty.c"
#include "bindings/fs.c"
#include "bindings/crypto.c"
#include "bindings/tls.c"
#include "bindings/threads.c"

#ifdef _WIN32
    #define PLATFORM "win32"
#elif __APPLE__
    
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        // define something for simulator   
    #elif TARGET_OS_IPHONE
        // define something for iphone  
    #else
        #define TARGET_OS_OSX 1
        // define something for OSX
    #endif
#elif __linux
     #define PLATFORM "linux"
#elif __unix // all unices not caught above
     #define PLATFORM "unix"
#elif __posix
    // POSIX
#endif

#endif /*_COMO_CORE_H*/
