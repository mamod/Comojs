
typedef struct {
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
} evAPI;

int io_start (evHandle* handle, int fd, int mask){
    if (_is_active(handle)) return 0;
    
    evIO *io = malloc(sizeof(*io));
    
    io->fd = fd;
    io->handle = handle;
    io->mask = mask;
    handle->ev = io;
    handle->type = EV_IO;
    QUEUE_INSERT_TAIL(&(handle)->loop->io_queue, &(io)->queue);
    
    evAPI *api = handle->loop->api;
    if (mask & EV_POLLIN)  FD_SET(fd, &api->rfds);
    if (mask & EV_POLLOUT) FD_SET(fd, &api->rfds);
    
    handle_start(handle);
    return 0;
}

int io_stop (evHandle* handle, int mask){
    if (!_is_active(handle)) return 0;
    evLoop *loop = handle->loop;
    evAPI *api = loop->api;
    evIO *io = handle->ev;
    
    if (mask & EV_POLLIN) {
        io->mask &= ~EV_POLLIN;
        FD_CLR(io->fd,&api->rfds);
    }
    
    if (mask & EV_POLLOUT) {
        io->mask &= ~EV_POLLOUT;
        FD_CLR(io->fd,&api->wfds);
    }
    
    if (!io->mask){
        QUEUE_REMOVE(&(io)->queue);
        handle_stop(handle);
        free(io);
        free(handle);
    }
    
    return 0;
}

static int _ev_api_create(evLoop *loop) {
    evAPI *state = malloc(sizeof(*state));
    if (!state) return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    loop->api = state;
    return 0;
}

static void io_poll (evLoop *loop, int timeout) {
    
    evAPI *api = loop->api;
    int retval, j, numevents = 0;
    uint64_t base;
    uint64_t diff;
    base = loop->time;
    
    POLLAGAIN : {
        
        memcpy(&api->_rfds,&api->rfds,sizeof(fd_set));
        memcpy(&api->_wfds,&api->wfds,sizeof(fd_set));
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeout*1000;
        
        retval = select(1024, &api->_rfds, &api->_wfds, NULL,
                         timeout == -1 ? NULL : &tv);
        
        if (retval > 0){
            QUEUE *q;
            evIO *io;
            int nevents = 0;
            QUEUE_FOREACH(q, &loop->io_queue) {
                int mask = 0;
                io = QUEUE_DATA(q, evIO, queue);
                
                if (io->mask & EV_POLLIN && FD_ISSET(io->fd,&api->_rfds))
                    mask |= EV_POLLIN;
                if (io->mask & EV_POLLOUT && FD_ISSET(io->fd,&api->_wfds))
                    mask |= EV_POLLOUT;
                
                if (mask != 0){
                    io->handle->cb(io->handle, mask);
                    nevents++;
                }
                
                if (nevents == retval){
                    break;
                }
            }
        }
    }
    
    if (retval == -1){
        printf("ERROR : %i\n", errno);
    }
    
    if (QUEUE_EMPTY(&loop->io_queue) || timeout == 0){
        return;
    }
    
    if (timeout == -1){
        goto POLLAGAIN;
    }
    
    loop_update_time(loop);
    diff = loop->time - base;
    if (diff < (uint64_t) timeout) {
        timeout -= diff;
        goto POLLAGAIN;
    }
    
    return;
}
