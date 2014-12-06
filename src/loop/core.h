#ifndef _EV_FDSET_H
#define _EV_FDSET_H

#if !defined( IO_FD_SETSIZE )
    #define IO_FD_SETSIZE 1024
#endif

#ifdef IO_FD_SETSIZE
    #ifdef __FD_SETSIZE
        #undef __FD_SETSIZE
        #define __FD_SETSIZE IO_FD_SETSIZE
    #else
        #undef FD_SETSIZE
        #define FD_SETSIZE IO_FD_SETSIZE
    #endif
#endif
#endif /* _EV_FDSET_H */

#include <stdlib.h>

#define HANDLE_REF       0x20
#define HANDLE_CLOSING   0x01
#define HANDLE_CLOSED    0x10
#define HANDLE_INTERNAL  0x80
#define HANDLE_ACTIVE    0x40

#define EV_TIMER         0x20
#define EV_IO            0x40

#define EV_POLLIN        0x20
#define EV_POLLOUT       0x40
#define EV_POLLERR       0x80

#ifdef _WIN32
    #ifdef __TINYC__
        #include "tinycc/winsock2.h"
        #include "tinycc/assert.h"
    #else
        #include <assert.h>
        #include <winsock2.h>
    #endif
    #include <windows.h>
#else
    #include <assert.h>
#endif

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#define _is_active(h)                \
  (((h)->flags & HANDLE_ACTIVE) != 0)


/*=============================================================================
  Loop stuct 
 ============================================================================*/
typedef struct {
    void* data;
    unsigned int active_handles;
    void *handle_queue[2];
    void *closing_queue[2];
    void *io_queue[2];
    void *api;
    unsigned int stop;
    uint64_t timer_counter;
    uint64_t time;
    struct {
        void *min;
        unsigned int nelts;
    } timer_heap;
} evLoop;

/*=============================================================================
  Handle stuct 
 ============================================================================*/
typedef struct evHandle {
    void *data;
    void *ev;
    void (*cb)(struct evHandle *, ...);
    void (*close)(struct evHandle *);
    void *queue[2];
    int flags;
    int type;
    evLoop *loop;
} evHandle;

/*=============================================================================
  Timers stuct 
 ============================================================================*/
typedef struct {
    evHandle *handle;
    uint64_t start_id;
    uint64_t timeout;
    uint64_t repeat;
    void *heap_node[3];
} evTimer;

/*=============================================================================
  IO stuct 
 ============================================================================*/
typedef struct {
    evHandle *handle;
    int mask;
    int fd;
    void *queue[2];
} evIO;

/*=============================================================================
  IO stuct 
============================================================================*/
typedef struct {
    evHandle *handle;
    int type;
    void *queue[2];
} evNextTick;

void loop_update_time (evLoop *loop);
void handle_ref (evHandle *h);
void handle_unref (evHandle *h);


int timer_start (evHandle* handle,uint64_t timeout,uint64_t repeat);
int timer_stop  (evHandle* handle);
int timer_again (evHandle* handle);
int timer_close (evHandle* handle);

int io_remove (evHandle* handle, int mask);
int io_add (evHandle* handle, int mask);
int io_start (evHandle* handle, int fd, int mask);
int io_stop (evHandle* handle, int mask);
int io_active (evHandle* handle, int mask);

void loop_update_time (evLoop *loop);
int loop_start (evLoop *loop, int type);
int loop_close (evHandle *handle, void *cb);

evLoop *loop_init ();
evLoop *main_loop ();
evHandle *handle_init (evLoop *loop, void *cb);
