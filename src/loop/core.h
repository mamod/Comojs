#include <stdlib.h>

#define HANDLE_REF       0x20
#define HANDLE_CLOSING   0x01
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
    #else
        #include <winsock2.h>
    #endif
    #include <windows.h>
#endif

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#define _is_active(h)                \
  (((h)->flags & HANDLE_ACTIVE) != 0)

typedef struct {
    void* data;
    unsigned int active_handles;
    void* handle_queue[2];
    void* closing_queue[2];
    void* io_queue[2];
    void *api;
    unsigned int stop;
    uint64_t timer_counter;
    uint64_t time;
    struct {
        void* min;
        unsigned int nelts;
    } timer_heap;
} evLoop;

typedef struct evHandle {
    void* data;
    void* ev;
    void (*cb)(struct evHandle *, ...);
    void* handle_queue[2];
    int flags;
    int type;
    evLoop* loop;
} evHandle;

typedef struct {
    evHandle *handle;
    uint64_t start_id;
    uint64_t timeout;
    uint64_t repeat;
    void* heap_node[3];
} evTimer;

typedef struct {
    evHandle *handle;
    int mask;
    int fd;
    void* queue[2];
} evIO;

void loop_update_time (evLoop *loop);
void handle_ref (evHandle *h);
void handle_unref (evHandle *h);


int timer_start (evHandle* handle,uint64_t timeout,uint64_t repeat);
int timer_stop (evHandle* handle);

int timer_close (evHandle* handle);
int io_start (evHandle* handle, int fd, int mask);
int io_stop (evHandle* handle, int mask);
void loop_update_time (evLoop *loop);
int loop_start (evLoop *loop, int type);
evLoop *loop_init ();
evLoop *main_loop ();
evHandle *handle_init (evLoop *loop, void *cb);
