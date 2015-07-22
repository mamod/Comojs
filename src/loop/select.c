
typedef struct {
    fd_set rfds, wfds, efds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds, _efds;
} evAPI;

int io_active (evHandle* handle, int mask) {
    evIO *io = handle->ev;
    assert(0 == (mask & ~(EV_POLLIN | EV_POLLOUT)));
    assert(0 != mask);
    return 0 != (io->mask & mask);
}

int io_add (evHandle* handle, int mask) {
    
    assert(handle && handle->type == EV_IO && "not IO handle");
    assert(handle->ev != NULL);

    evIO *io = handle->ev;
    assert(io->fd > -1);

    evAPI *api = handle->loop->api;
    io->mask   = io->mask | mask;
    
    int fd = io->fd;
    if (mask & EV_POLLIN)  FD_SET(fd, &api->rfds);
    if (mask & EV_POLLOUT) FD_SET(fd, &api->wfds);
    if (mask & EV_POLLERR) FD_SET(fd, &api->efds);
    
    return 0;
}

int io_remove (evHandle* handle, int mask) {
    assert(handle && handle->type == EV_IO);
    if (!_is_active(handle)) return 0;

    assert(handle->ev != NULL);

    evLoop *loop = handle->loop;
    evAPI  *api  = loop->api;
    evIO   *io   = handle->ev;

    assert(io->fd > -1);

    if (mask & EV_POLLIN) {
        io->mask &= ~EV_POLLIN;
        FD_CLR(io->fd, &api->rfds);
    }
    
    if (mask & EV_POLLOUT) {
        io->mask &= ~EV_POLLOUT;
        FD_CLR(io->fd, &api->wfds);
    }

    if (mask & EV_POLLERR) {
        io->mask &= ~EV_POLLERR;
        FD_CLR(io->fd, &api->efds);
    }

    return 0;
}

int io_start (evHandle* handle, int fd, int mask){
    assert(handle != NULL);
    if (handle->flags & HANDLE_CLOSING) return 0;
    if (_is_active(handle)) return io_add(handle, mask);

    evIO *io = malloc(sizeof(*io));
    memset(io, 0, sizeof(*io));

    io->fd       = fd;
    io->handle   = handle;
    io->mask     = mask;

    handle->ev   = io;
    handle->type = EV_IO;
    QUEUE_INIT(&io->queue);
    QUEUE_INSERT_TAIL(&handle->loop->io_queue, &io->queue);
    
    evAPI *api = handle->loop->api;
    if (mask & EV_POLLIN)  FD_SET(fd, &api->rfds);
    if (mask & EV_POLLOUT) FD_SET(fd, &api->wfds);
    if (mask & EV_POLLERR) FD_SET(fd, &api->efds);

    handle_start(handle);
    return 0;
}

int io_stop (evHandle* handle, int mask){
    // if (handle->flags & HANDLE_CLOSING) return 0;
    if (!_is_active(handle)) return 0;

    assert(handle->ev != NULL);
    evIO *io = handle->ev;
    io_remove(handle, mask);
    
    if (!io->mask){
        handle->flags |= HANDLE_CLOSING;
        handle_stop(handle);
        QUEUE_REMOVE(&io->queue);
        QUEUE_INSERT_TAIL(&handle->loop->closing_queue, 
                          &handle->queue);
    }
    
    return 0;
}

int io_close (evHandle* handle) {
    return io_stop(handle, EV_POLLOUT | EV_POLLIN | EV_POLLERR);
}

static int loop_poll_create(evLoop *loop) {
    evAPI *state = malloc(sizeof(*state));
    if (!state) return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    FD_ZERO(&state->efds);
    loop->api = state;
    return 0;
}

static void io_poll (evLoop *loop, int timeout) {
    
    evAPI *api = loop->api;
    int retval = 0;

    memcpy(&api->_rfds,&api->rfds,sizeof(fd_set));
    memcpy(&api->_wfds,&api->wfds,sizeof(fd_set));
    memcpy(&api->_efds,&api->efds,sizeof(fd_set));

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout * 1000;
    errno = 0;

    /* FIXME: add a max fd value to io struct */
    retval = select(1024, &api->_rfds, &api->_wfds, &api->_efds,
                     timeout == -1 ? NULL : &tv);
    
    #ifdef _WIN32
        errno = WSAGetLastError();
    #endif

    if (retval > 0){
        QUEUE *q;
        int nevents = 0;
        QUEUE_FOREACH(q, &loop->io_queue) {
            
            assert( q != NULL );
            evIO *io = QUEUE_DATA(q, evIO, queue);
            assert(io != NULL);

            int mask = 0;
            if (io->mask & EV_POLLIN && FD_ISSET(io->fd, &api->_rfds)) {
                mask |= EV_POLLIN;
            }

            if (io->mask & EV_POLLOUT && FD_ISSET(io->fd, &api->_wfds)) {
                mask |= EV_POLLOUT;
            }

            if (io->mask & EV_POLLERR && FD_ISSET(io->fd, &api->_efds)) {
                mask |= EV_POLLERR;
            }
            
            if (mask != 0){
                if (io->handle->cb != NULL) {
                    io->handle->cb(io->handle, mask);
                }
                nevents++;
            }
            
            /* break once we match number of events */
            if (nevents == retval){
                break;
            }
        }
    }
    
    if (retval == -1){
        /*
         on windows we may get an EINVAL error if we have all fd sets nulled
         this is an odd behaviour so we need to overcome a loop saturation 
        */
        #ifdef _WIN32
            if (errno == WSAEINVAL){
                Sleep(1);
                return;
            }
        #endif

        assert(0 && "select error");
        return;
    }
    
    if (QUEUE_EMPTY(&loop->io_queue) || timeout == 0){
        return;
    }
    
    return;
}
