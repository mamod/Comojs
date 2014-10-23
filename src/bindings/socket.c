#include <fcntl.h>
#include "inet.c"

#ifdef _WIN32
    #define GetSockError WSAGetLastError()
#else
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define GetSockError errno
#endif

/** 
  * pton4
******************************************************************************/
static const int _sock_inet_pton4(duk_context *ctx) {
    const char *ip = duk_require_string(ctx,0);
    int port       = duk_require_int(ctx,1);
    
    struct sockaddr_in *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    
    int ret = uv_inet_pton(AF_INET, ip, &(addr->sin_addr.s_addr));
    if (ret){
        free(addr);
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_pointer(ctx, addr);
    return 1;
}

/** 
  * pton6
******************************************************************************/
static const int _sock_inet_pton6(duk_context *ctx) {
    const char *ip = duk_require_string(ctx,0);
    int port       = duk_require_int(ctx,1);
    
    struct sockaddr_in6 *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));
    
    char address_part[40];
    size_t address_part_size;
    const char* zone_index;
    
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    
    zone_index = strchr(ip, '%');
    if (zone_index != NULL) {
        address_part_size = zone_index - ip;
        if (address_part_size >= sizeof(address_part))
        address_part_size = sizeof(address_part) - 1;
        memcpy(address_part, ip, address_part_size);
        address_part[address_part_size] = '\0';
        ip = address_part;
        zone_index++; /* skip '%' */
        /* NOTE: unknown interface (id=0) is silently ignored */
        #ifdef _WIN32
            addr->sin6_scope_id = atoi(zone_index);
        #else
            addr->sin6_scope_id = if_nametoindex(zone_index);
        #endif
    }
    
    int ret = uv_inet_pton(AF_INET6, ip, &(addr->sin6_addr));
    if (ret){
        free(addr);
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_pointer(ctx, addr);
    return 1;
}

/** 
  * pton
******************************************************************************/
static const int _sock_inet_pton(duk_context *ctx) {
    errno = 0;
    int ret = _sock_inet_pton4(ctx);
    if (errno == EINVAL){
        COMO_SET_ERRNO(ctx, 0); //reset errno
        ret = _sock_inet_pton6(ctx);
    }
    return ret;
}

/** 
  * ntop4
******************************************************************************/
static const int _sock_inet_ntop4(duk_context *ctx) {
    struct sockaddr_in *addr = duk_require_pointer(ctx,0);
    
    char dst[32];
    int ret = uv_inet_ntop(AF_INET, &addr->sin_addr,
                           (char *)dst, sizeof(dst));
    
    if (ret){
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_string(ctx, dst);
    return 1;
}

/** 
  * ntop6
******************************************************************************/
static const int _sock_inet_ntop6(duk_context *ctx) {
    struct sockaddr_in6 *addr = duk_require_pointer(ctx,0);
    
    char dst[128];
    int ret = uv_inet_ntop(AF_INET6, &addr->sin6_addr, (char *)dst,
                           sizeof(dst));
    
    if (ret){
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_string(ctx, dst);
    return 1;
}

/** 
  * ntop
******************************************************************************/
static const int _sock_inet_ntop(duk_context *ctx) {
    struct sockaddr *addr = duk_require_pointer(ctx,0);
    
    if (addr->sa_family == AF_INET){
        return _sock_inet_ntop4(ctx);
    }
    
    else if (addr->sa_family == AF_INET6){
        return _sock_inet_ntop6(ctx);
    }
    
    else {
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
}

/** 
  * family
******************************************************************************/
static const int _sock_inet_family(duk_context *ctx) {
    struct sockaddr *addr = duk_require_pointer(ctx,0);
    
    duk_push_int(ctx, addr->sa_family);
    return 1;
}

/** 
  * socket
******************************************************************************/
static const int _sock_socket(duk_context *ctx) {
    int af       = duk_require_int(ctx, 0);
    int type     = duk_require_int(ctx, 1);
    int protocol = duk_require_int(ctx,2);
    
    int iResult = 0;
    #ifdef _WIN32
        WSADATA wsaData = {0};
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            COMO_SET_ERRNO_AND_RETURN(ctx, iResult);
        }
    #endif
    
    SOCKET sock = socket(af, type, protocol);
    if (sock == INVALID_SOCKET){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_int(ctx, sock);
    return 1;
}

/** 
  * setsockopt
******************************************************************************/
static const int _sock_setsockopt(duk_context *ctx) {
    SOCKET s    = duk_get_int(ctx, 0);
    int level   = duk_require_int(ctx, 1);
    int optname = duk_require_int(ctx,2);
    int optval  = duk_require_int(ctx,3);
    
    int optlen = sizeof(optval);
    if (setsockopt(s, level, optname, (char *) &optval, optlen) ==
         SOCKET_ERROR){
        
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** 
  * bind
******************************************************************************/
static const int _sock_bind(duk_context *ctx) {
    SOCKET s              = duk_require_int(ctx, 0);
    struct sockaddr *addr = duk_require_pointer(ctx,1);
    
    unsigned int addrlen;
    if (addr->sa_family == AF_INET){
        addrlen = sizeof(struct sockaddr_in);
    } else if (addr->sa_family == AF_INET6){
        addrlen = sizeof(struct sockaddr_in6);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
    
    if (bind(s, addr, addrlen) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}


/** listen for connections on a socket, called from javascript land, with 2
   * arguments a pointer to a packed ip address struct and a backlog number
   * of connections socket can listen to
   *
   * @param {Pointer} pointer to a packed ip address
   * @param {Number} backlog number of max connections
   * 
******************************************************************************/
static const int _sock_listen(duk_context *ctx) {
    SOCKET s    = duk_require_int(ctx, 0);
    int backlog = duk_require_int(ctx, 1);
    
    if (listen(s, backlog) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** initiate a connection on a socket, called from javascript land, with 2
  * arguments a socket handle/fd and packed ip address pointer
  *
  * @param {SOCK} fd/handle of the socket to be connected 
  * @param {Pointer} packed ip address
  * 
******************************************************************************/
static const int _sock_connect(duk_context *ctx) {
    SOCKET s              = duk_require_int(ctx, 0);
    struct sockaddr *addr = duk_require_pointer(ctx,1);
    
    unsigned int addrlen;
    if (addr->sa_family == AF_INET){
        addrlen = sizeof(struct sockaddr_in);
    } else if (addr->sa_family == AF_INET6){
        addrlen = sizeof(struct sockaddr_in6);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
    
    if (connect(s, addr, addrlen) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** accept a connection on a socket , called from javascript land,
  * with 1 argument, a socket fd/handle
  *
  * @param {SOCKET} socket fd/handle
  * 
******************************************************************************/
static const int _sock_accept(duk_context *ctx) {
    SOCKET s = duk_require_int(ctx, 0);
    
    SOCKET peer = accept(s, NULL, NULL);
    if (peer ==  INVALID_SOCKET){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    duk_push_int(ctx, peer);
    return 1;
}

int como_core_nonblock(int fd, int set) {
    int flags;
    int r;
    #ifdef _WIN32
        u_long iMode = set;
        r = ioctlsocket(fd, FIONBIO, &iMode);
        if ( r != 0 ){
            return WSAGetLastError();
        }
    #else
        do {
            r = fcntl(fd, F_GETFL);
        } while (r == -1 && errno == EINTR);
        
        if (r == -1) {
            return errno;
        }
        
        /* Bail out now if already set/clear. */
        if (!!(r & O_NONBLOCK) == !!set) {
            return 0;
        }
        
        if (set) {
            flags = r | O_NONBLOCK;
        } else {
            flags = r & ~O_NONBLOCK;
        }
        
        do {
            r = fcntl(fd, F_SETFL, flags);
        } while (r == -1 && errno == EINTR);
        
        if (r) return errno;
    #endif
    return 0;
}

/** mark open socket as non-blocking or blocking , called from javascript 
  * land, accepts 2 argument, a socket fd/handle, a set (1|0)
  *
  * @param {SOCKET} socket fd/handle
  * @param {SET} (0|1) 1 for marking socket as non blocking, 0 for blocking
  * 
******************************************************************************/
static const int _sock_nonblock(duk_context *ctx) {
    SOCKET s = duk_require_int(ctx, 0);
    int set  = duk_require_int(ctx, 1);

    int ret  = como_core_nonblock(s,set);
    if (ret) {
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** 
  * recv
******************************************************************************/
static const int _sock_recv (duk_context *ctx) {
    SOCKET fd  = duk_require_int(ctx, 0);
    size_t len = (size_t)duk_require_int(ctx, 1);
    int flags  = duk_get_int(ctx, 2);
    
    char *buf = duk_push_fixed_buffer(ctx, len);
    size_t n = recv(fd, buf, len, flags);
    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    if (n < len) duk_push_lstring(ctx, buf, n);
    return 1;
}

/** 
  * send
******************************************************************************/
static const int _sock_send (duk_context *ctx) {
    SOCKET fd       = duk_require_int(ctx, 0);
    const char *str = duk_require_string(ctx, 1);
    size_t len      = (size_t)duk_require_int(ctx, 2);
    int flags       = duk_require_int(ctx, 3);
    
    char *buf = duk_push_fixed_buffer(ctx, len);
    size_t n = send(fd, str, len, flags);
    if (n == -1){
        printf("ERROR : %d\n", GetSockError);
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    duk_push_int(ctx, (size_t)n);
    return 1;
}

/** 
  * shutdown
******************************************************************************/
static const int _sock_shutdown (duk_context *ctx) {
    SOCKET s = duk_require_int(ctx, 0);
    int how  = duk_get_int(ctx, 1);
    if (shutdown(s, how) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** 
  * close
******************************************************************************/
static const int _sock_close(duk_context *ctx) {
    SOCKET s = duk_require_int(ctx, 0);
    
    int ret;
    #ifdef _WIN32
        ret = closesocket(s);
    #else
        ret = close(s);
    #endif
    
    if (ret == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, GetSockError);
    }
    
    duk_push_true(ctx);
    return 1;
}

/** 
  * socket functions list
******************************************************************************/
static const duk_function_list_entry socket_funcs[] = {
    { "pton4"      , _sock_inet_pton4, 2 },
    { "pton6"      , _sock_inet_pton6, 2 },
    { "pton"       , _sock_inet_pton, 2 },
    { "ntop4"      , _sock_inet_ntop4, 1 },
    { "ntop6"      , _sock_inet_ntop6, 1 },
    { "ntop"       , _sock_inet_ntop, 1 },
    { "family"     , _sock_inet_family, 1 },
    { "socket"     , _sock_socket, 3 },
    { "setsockopt" , _sock_setsockopt, 4 },
    { "bind"       , _sock_bind, 2 },
    { "listen"     , _sock_listen, 2 },
    { "connect"    , _sock_connect, 3 },
    { "accept"     , _sock_accept, 1 },
    { "nonblock"   , _sock_nonblock, 2 },
    { "recv"       , _sock_recv, 3},
    { "send"       , _sock_send, 4},
    { "shutdown"   , _sock_shutdown,2 },
    { "close"      , _sock_close, 1 },
    { NULL         , NULL, 0 }
};

/** 
  * socket constants
******************************************************************************/
static const duk_number_list_entry socket_constants[] = {
    
    /* socket domain constants */
    { "AF_INET"     , AF_INET },
    { "AF_INET6"    , AF_INET6 },
    
    { "AF_UNIX"     , AF_UNIX },
    { "AF_INET6"    , AF_INET6 },
    { "AF_INET6"    , AF_INET6 },
    { "AF_INET6"    , AF_INET6 },
    { "AF_INET6"    , AF_INET6 },
    
    /* socket type constants */
    { "SOCK_STREAM"    , SOCK_STREAM },
    { "SOCK_SEQPACKET" , SOCK_SEQPACKET },
    { "SOCK_RAW"       , SOCK_RAW },
    { "SOCK_DGRAM"     , SOCK_DGRAM },
    { "SOCK_RDM"     , SOCK_RDM },
    #ifdef SOCK_NONBLOCK
    { "SOCK_NONBLOCK" , SOCK_NONBLOCK },
    #endif
    #ifdef SOCK_CLOEXEC
    { "SOCK_CLOEXEC" , SOCK_CLOEXEC },
    #endif
    
    /* socket options */
    //SOL_SOCKET level
    { "SOL_SOCKET"  , SOL_SOCKET },
    { "SO_DEBUG ", SO_DEBUG },
    { "SO_BROADCAST ", SO_BROADCAST },
    { "SO_KEEPALIVE ", SO_KEEPALIVE },
    { "SO_LINGER ", SO_LINGER },
    { "SO_SNDBUF ", SO_SNDBUF },
    { "SO_RCVBUF ", SO_RCVBUF },
    { "SO_RCVTIMEO ", SO_RCVTIMEO },
    { "SO_SNDTIMEO ", SO_SNDTIMEO },
    { "SO_SNDLOWAT ", SO_SNDLOWAT },
    { "SO_RCVLOWAT ", SO_RCVLOWAT },
    { "SO_REUSEADDR", SO_REUSEADDR},
    
    #ifdef SO_EXCLUSIVEADDRUSE
    { "SO_EXCLUSIVEADDRUSE ", SO_EXCLUSIVEADDRUSE },
    #endif
    
    #ifdef SO_NOSIGPIPE
    { "SO_NOSIGPIPE", SO_NOSIGPIPE },
    #endif
    
    //IPPROTO_TCP level
    {"IPPROTO_TCP"  , IPPROTO_TCP},
    { "TCP_NODELAY" , TCP_NODELAY },
    
    /* socket shutdown constants */
    #ifdef SD_RECEIVE
    {"SHUT_RD"      , SD_RECEIVE},
    {"SD_RECEIVE"   , SD_RECEIVE},
    #else
    {"SHUT_RD"      , SHUT_RD},
    {"SD_RECEIVE"   , SHUT_RD},
    #endif
    
    #ifdef SD_SEND
    {"SHUT_WR"      , SD_SEND},
    {"SD_SEND"      , SD_SEND},
    #else
    {"SHUT_WR"      , SHUT_WR},
    {"SD_SEND"      , SHUT_WR},
    #endif
    
    #ifdef SD_BOTH
    {"SD_BOTH"      , SD_BOTH},
    {"SHUT_RDWR"    , SD_BOTH},
    #else
    {"SD_BOTH"      , SHUT_RDWR},
    {"SHUT_RDWR"    , SHUT_RDWR},
    #endif
    
    { "SOMAXCONN"   , SOMAXCONN },
    { NULL, 0 }
};

/** 
  * socket function initiation
******************************************************************************/
static const int init_binding_socket(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, socket_funcs);
    duk_put_number_list(ctx, -1, socket_constants);
    return 1;
}
