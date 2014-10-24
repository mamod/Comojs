#include "duktape/duktape.h"
#include "thread/tinycthread.h"

#include <assert.h>
#include <errno.h>
#define _COMO_CORE

#ifdef _WIN32
    #ifdef __TINYC__
        #include "tinycc/winsock2.h"
        #include "tinycc/mswsock.h"
        #include "tinycc/ws2tcpip.h"
    #else
        #include <winsock2.h>
        #include <mswsock.h>
        #include <ws2tcpip.h>
    #endif

    #include <windows.h>
    #define dlopen(x,y) (void*)LoadLibrary(x)
    #define dlclose(x) FreeLibrary((HMODULE)x)
#else
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
#endif

/* Need to define _wassert as tcc doesn't recognize it on win32 ?!!*/
#ifndef _wassert
#define _wassert assert
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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
    return DUK_RET_ERROR;      \
} while(0)

void create_new_thread_heap (duk_context *ctx, const char *worker);

como_platform_sleep (int timeout){
    #ifdef _WIN32
        Sleep(timeout);
    #else
        usleep(1000 * timeout);
    #endif
}

/* BINDINGS */
#include "bindings/errno.c"
#include "bindings/buffer.c"
#include "bindings/socket.c"
#include "bindings/io.c"
#include "bindings/loop.c"
#include "bindings/thread.c"
#include "bindings/http-parser.c"
#include "bindings/readline.c"

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
