#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include "heap.h"
#include "queue.h"
#include "core.h"

inline void handle_start (evHandle *h);
inline void handle_stop (evHandle *h);

#include "select.c"

inline void handle_start (evHandle *h){
    if ((h->flags & HANDLE_ACTIVE) != 0) return;
    h->flags |= HANDLE_ACTIVE;
    if ((h->flags & HANDLE_REF) != 0) {
        h->loop->active_handles++;
    }
}

inline void handle_stop (evHandle *h) {
    if ((h->flags & HANDLE_ACTIVE) == 0) return;
    h->flags &= ~HANDLE_ACTIVE;
    if ((h->flags & HANDLE_REF) != 0) {
        h->loop->active_handles--;
    }
}

void handle_ref (evHandle *h) {
    if ((h->flags & HANDLE_REF) != 0) return;
    h->flags |= HANDLE_REF;
    if ((h->flags & HANDLE_CLOSING) != 0) return;
    if ((h->flags & HANDLE_ACTIVE) != 0) h->loop->active_handles++;
}

void handle_unref (evHandle *h) {
    if ((h->flags & HANDLE_REF) == 0) return;
    h->flags &= ~HANDLE_REF;
    if ((h->flags & HANDLE_CLOSING) != 0) return;
    if ((h->flags & HANDLE_ACTIVE) != 0) h->loop->active_handles--;
}

evHandle *handle_init (evLoop *loop, void *cb) {
    evHandle *handle = malloc(sizeof(*handle));
    handle->loop = loop;
    
    if (cb != NULL){
        handle->cb = cb;
    }
    
    handle->type = 0;
    handle->flags = HANDLE_REF;
    //QUEUE_INSERT_TAIL(&(loop)->handle_queue, &(handle)->handle_queue);
    return handle;
}

/* Timers */
static int timer_less_than(const struct heap_node* ha,
                           const struct heap_node* hb) {
    const evTimer* a;
    const evTimer* b;
    
    a = container_of(ha, const evTimer, heap_node);
    b = container_of(hb, const evTimer, heap_node);
    
    if (a->timeout < b->timeout)
        return 1;
    if (b->timeout < a->timeout)
        return 0;
    
    /* Compare start_id when both have the same timeout. start_id is
     * allocated with loop->timer_counter in uv_timer_start().
    */
    if (a->start_id < b->start_id)
        return 1;
    if (b->start_id < a->start_id)
        return 0;
    
    return 0;
}

int next_timeout(const evLoop *loop) {
    const struct heap_node* heap_node;
    const evTimer *handle;
    uint64_t diff;
    
    heap_node = heap_min((const struct heap*) &loop->timer_heap);
    if (heap_node == NULL) return -1; /* block indefinitely */
    
    handle = container_of(heap_node, const evTimer, heap_node);
    if (handle->timeout <= loop->time) return 0;
    
    diff = handle->timeout - loop->time;
    if (diff > INT_MAX) diff = INT_MAX;
    return diff;
}

int timer_start (evHandle* handle,
                 uint64_t timeout,
                 uint64_t repeat){
    
    if (_is_active(handle)){
        timer_stop(handle);
    }
    
    evTimer *timer;
    if (!handle->type) {
        timer = malloc(sizeof(*timer));
    } else {
        timer = handle->ev;
    }
    
    uint64_t clamped_timeout = handle->loop->time + timeout;
    if (clamped_timeout < timeout) clamped_timeout = (uint64_t) -1;
    timer->timeout = clamped_timeout;
    timer->repeat = repeat;
    timer->start_id = handle->loop->timer_counter++;
    timer->handle = handle;
    handle->ev = timer;
    handle->type = EV_TIMER;
    heap_insert((struct heap*) &handle->loop->timer_heap,
              (struct heap_node*) &timer->heap_node,
              timer_less_than);
    
    handle_start(handle);
    return 1;
}

int timer_stop (evHandle* handle) {
    if (!handle || !_is_active(handle)) return 0;
    evTimer *timer = handle->ev;
    heap_remove((struct heap*) &handle->loop->timer_heap,
              (struct heap_node*) &timer->heap_node,
              timer_less_than);
    
    handle_stop(handle);
    return 0;
}

int timer_close (evHandle* handle) {
    handle->flags |= HANDLE_CLOSING;
}

int timer_again(evHandle* handle) {
    timer_stop(handle);
    if (handle->cb == NULL) return 0;
    evTimer *timer = handle->ev;
    if (timer->repeat) {
        timer_start(handle, timer->repeat, timer->repeat);
    }
    return 0;
}

void run_timers(evLoop* loop) {
    struct heap_node* heap_node;
    evTimer* timer;
    for (;;) {
        heap_node = heap_min((struct heap*) &loop->timer_heap);
        if (heap_node == NULL) break;
        
        timer = container_of(heap_node, evTimer, heap_node);
        if (timer->timeout > loop->time) {
            break;
        }
        
        timer_again(timer->handle);
        timer->handle->cb(timer->handle);
    }
}

void loop_update_time (evLoop *loop){
    #ifdef _WIN32
        loop->time = GetTickCount();
    #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        loop->time = tv.tv_sec * 1000 + tv.tv_usec/1000;
    #endif
}

evLoop *gLoop;
evLoop *loop_init (){
    evLoop *loop = malloc(sizeof(*loop));
    memset(loop, 0, sizeof(*loop));
    loop->active_handles = 0;
    loop->stop = 0;
    loop->time = 0;
    loop->timer_counter = 0;
    loop_update_time(loop);
    _ev_api_create(loop);
    heap_init((struct heap*) &loop->timer_heap);
    QUEUE_INIT(&loop->handle_queue);
    QUEUE_INIT(&loop->closing_queue);
    QUEUE_INIT(&loop->io_queue);
    
    if (gLoop == NULL){
        gLoop = loop;
    }
    
    return loop;
}

evLoop *main_loop () {
    if (gLoop == NULL){
        gLoop = loop_init();
    }
    return gLoop;
}

void _make_close_pending(evHandle *handle) {
    assert(handle->flags & HANDLE_CLOSING);
    assert(!(handle->flags & HANDLE_CLOSED));
    evLoop *loop = handle->loop;
    QUEUE_INSERT_TAIL(&(loop)->closing_queue, &(handle)->handle_queue);
    //handle->next_closing = loop->closing_handles;
    //loop->closing_handles = handle;
}

int loop_close (evHandle *handle, void *close_cb) {
    handle->flags |= HANDLE_CLOSING;
    handle->cb = close_cb;
    switch (handle->type) {
        case EV_TIMER: {
            timer_stop(handle);
            break;
        };

        case EV_IO: {
            //uv__pipe_close((uv_pipe_t*)handle);
            printf("closing IO\n");
            break;
        };

        default : {
            printf("closing IO %i\n", handle->type);
            break;
        }
    }

    _make_close_pending(handle);
    return 1;
}

static void _finish_close(evHandle* handle) {
    /* Note: while the handle is in the UV_CLOSING state now, it's still possible
    * for it to be active in the sense that uv__is_active() returns true.
    * A good example is when the user calls uv_shutdown(), immediately followed
    * by uv_close(). The handle is considered active at this point because the
    * completion of the shutdown req is still pending.
    */
    assert(handle->flags & HANDLE_CLOSING);
    assert(!(handle->flags & HANDLE_CLOSED));
    handle->flags |= HANDLE_CLOSED;

    switch (handle->type) {
        case EV_IO:
        case EV_TIMER:
            break;

        default:
            assert(0);
            break;
    }

    handle_unref(handle);
    QUEUE_REMOVE(&handle->handle_queue);
    if (handle->cb) {
        handle->cb(handle);
    }

    handle->cb = NULL;
    free(handle->data);
    free(handle);
}

static void _run_closing_handles(evLoop *loop) {

    QUEUE *q;
    evHandle *w;

    while (!QUEUE_EMPTY(&loop->closing_queue)) {
        q = QUEUE_HEAD(&loop->closing_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        w = QUEUE_DATA(q, evHandle, handle_queue);
        _finish_close(w);
    }
}

int loop_start (evLoop *loop, int type){
    while (loop->active_handles){
        loop_update_time(loop);
        int timeout = next_timeout(loop);
        run_timers(loop);
        _run_closing_handles(loop);

        if (QUEUE_EMPTY(&loop->io_queue)) {
            #ifdef _WIN32
                Sleep(timeout);
            #else
                usleep(1000 * timeout);
            #endif
        } else {
            io_poll(loop, timeout);
        }
        
        if (type == 1){
            break;
        }
    }
    return 0;
}
