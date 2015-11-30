#include <fcntl.h>
#include "como_errno.h"
#ifndef _WIN32
    #include <net/if.h>
#endif

struct in6_addr como_anyaddr6  = IN6ADDR_ANY_INIT;
struct in6_addr como_loopback6 = IN6ADDR_LOOPBACK_INIT;

/*=============================================================================
  getprotobyname Return proto num id from name string 
  socket.getprotobyname('tcp');
 ============================================================================*/
COMO_METHOD(como_sock_getprotobyname) {
    const char* name = duk_require_string(ctx, 0);

    struct protoent *proto = getprotobyname(name);

    if (!proto){
        COMO_SET_ERRNO_AND_RETURN(ctx, errno);
    }

    duk_push_int(ctx, proto->p_proto);
    return 1;
}

/*=============================================================================
  Returns array of ip address and port of address structure
 ============================================================================*/
const int ix = 1;
#define como_bigendian() ( (*(char*)&ix) == 0 )

unsigned short swaps( unsigned short val) {
    return ((val & 0xff) << 8) | ((val & 0xff00) >> 8);
}

COMO_METHOD(como_sock_address_info) {
    struct sockaddr *addr = duk_require_pointer(ctx, 0);

    duk_idx_t arr_idx = duk_push_array(ctx);

    char dst[128];
    switch (addr->sa_family) {
        #ifndef _WIN32
            case AF_UNIX: {
                // struct sockaddr_un * addr_un = (struct sockaddr_un *) addr;
                // duk_push_string(ctx, addr_un->sun_path);
                // duk_put_prop_index(ctx, arr_idx, 0);
                duk_push_null(ctx);
                duk_put_prop_index(ctx, arr_idx, 1);
                return 1;
            } break;
        #endif
        
        case AF_INET6: {
            struct sockaddr_in6 *addr_in = (struct sockaddr_in6 *) addr;
            int ret = uv_inet_ntop(AF_INET6, &addr_in->sin6_addr,
                                    (char *)dst,
                                    sizeof(dst));

            if (ret) COMO_SET_ERRNO_AND_RETURN(ctx, ret);
            duk_push_int(ctx, addr_in->sin6_port);
            duk_put_prop_index(ctx, arr_idx, 1);
        } break;
        
        case AF_INET: {
            struct sockaddr_in * addr_in = (struct sockaddr_in *) addr;
            int ret = uv_inet_ntop(AF_INET, &addr_in->sin_addr.s_addr, 
                         (char *)dst, sizeof(dst));

            if (ret) COMO_SET_ERRNO_AND_RETURN(ctx, ret);
            if (como_bigendian()){
                duk_push_int(ctx, addr_in->sin_port);
            } else {
                duk_push_int(ctx, swaps(addr_in->sin_port));
            }
            
            duk_put_prop_index(ctx, arr_idx, 1);
        } break;
    }
    
    duk_push_string(ctx, dst);
    duk_put_prop_index(ctx, arr_idx, 0);

    return 1;
}

/*=============================================================================
  Returns family of ip structure
 ============================================================================*/
COMO_METHOD(como_sock_get_family) {
    struct sockaddr *addr = duk_require_pointer(ctx, 0);
    duk_push_int(ctx, addr->sa_family);
    return 1;
}

/*=============================================================================
  check if the given string represents a valid ip address
  socket.isIP('127.0.0.1');
  return 0 if not valid
  return 4/6 if valid
  4 => ipv4
  6 => ipv6
 ============================================================================*/
COMO_METHOD(como_sock_isIP) {
    const char *ip = duk_require_string(ctx, 0);

    int rc = 0;
    char address_buffer[sizeof(struct in6_addr)];
    if (uv_inet_pton(AF_INET, ip, &address_buffer) == 0)
        rc = 4;
    else if (uv_inet_pton(AF_INET6, ip, &address_buffer) == 0)
        rc = 6;
    
    duk_push_int(ctx, rc);
    return 1;
}

/*=============================================================================
  getpeername
  returns address structure of the peer connected to the socket
  socket.getpeername(sockfd);
 ============================================================================*/
COMO_METHOD(como_sock_getpeername) {
    SOCKET sock = duk_require_int(ctx, 0);
    struct sockaddr *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));
    
    socklen_t len = sizeof(struct sockaddr);

    int result = getpeername(sock, (struct sockaddr *) addr, &len);
    if (result == 0) {
        duk_push_pointer(ctx, (void *)addr);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    return 1;
}

/*=============================================================================
  Rerurns IP structire to which the socket sockfd is bound
  socket.getsockname(sockfd);
 ============================================================================*/
COMO_METHOD(como_sock_getsockname) {
    SOCKET sock = duk_require_int(ctx, 0);

    struct sockaddr_storage *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));

    socklen_t len = sizeof(struct sockaddr_storage);
    int result = getsockname(sock, (struct sockaddr *) addr, &len);
    if (result == 0) {
        duk_push_pointer(ctx, (void *)addr);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    return 1;
}

/*=============================================================================
  setsockopt - set options on sockets
 ============================================================================*/
COMO_METHOD(como_sock_setsockopt) {
    SOCKET s    = duk_get_int(ctx, 0);
    int level   = duk_require_int(ctx, 1);
    int optname = duk_require_int(ctx,2);
    int optval  = duk_require_int(ctx,3);
    
    int optlen = sizeof(optval);
    if (setsockopt(s, level, optname, (char *) &optval, optlen) ==
         SOCKET_ERROR){
        
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  getsockopt - get options on sockets
 ============================================================================*/
COMO_METHOD(como_sock_getsockopt) {
    SOCKET s    = duk_require_int(ctx, 0);
    int level   = duk_require_int(ctx, 1);
    int optname = duk_require_int(ctx,2);
    
    int valopt; 
    socklen_t lon = sizeof(valopt);
    if (getsockopt(s, level, optname, (char *)(&valopt), &lon) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }

    duk_push_int(ctx, valopt);
    return 1;
}

/*=============================================================================
  pton4
 ============================================================================*/
COMO_METHOD(como_sock_inet_pton4) {

    int port       = duk_require_int(ctx,1);
    
    struct sockaddr_in *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    const char *ip;
    if (duk_is_number(ctx, 0)) {
        int inaddr = duk_get_int(ctx,0);
        addr->sin_addr.s_addr = htonl(inaddr);
        goto end;
    } else {
        ip = duk_require_string(ctx,0);
    }

    int ret = uv_inet_pton(AF_INET, ip, &(addr->sin_addr.s_addr));
    if (ret){
        free(addr);
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }

    end:
        duk_push_pointer(ctx, addr);

    return 1;
}

/*=============================================================================
  pton6
 ============================================================================*/
COMO_METHOD(como_sock_inet_pton6) {

    int port = duk_require_int(ctx,1);
    
    struct sockaddr_in6 *addr = malloc(sizeof(*addr));
    memset(addr, 0, sizeof(*addr));
    
    char address_part[40];
    size_t address_part_size;
    const char* zone_index;
    
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);

    const char *ip;
    if (duk_is_number(ctx, 0)) {
        int inaddr = duk_get_int(ctx, 0);
        if (inaddr == INADDR_ANY) {
            addr->sin6_addr = como_anyaddr6;
        } else if (inaddr == INADDR_LOOPBACK){
            addr->sin6_addr = como_loopback6;
        }
        goto end;
    } else {
        ip = duk_require_string(ctx,0);
    }

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
    
    end:
        duk_push_pointer(ctx, addr);
    return 1;
}

/*=============================================================================
  pton
 ============================================================================*/
COMO_METHOD(como_sock_inet_pton) {
    errno = 0;
    int ret = como_sock_inet_pton4(ctx);
    if (errno == EINVAL){
        COMO_SET_ERRNO(ctx, 0); //reset errno
        ret = como_sock_inet_pton6(ctx);
    }
    return ret;
}

/*=============================================================================
  ntop4
 ============================================================================*/
COMO_METHOD(como_sock_inet_ntop4) {
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

/*=============================================================================
  ntop6
 ============================================================================*/
COMO_METHOD(como_sock_inet_ntop6) {
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

/*=============================================================================
  ntop
 ============================================================================*/
COMO_METHOD(como_sock_inet_ntop) {
    struct sockaddr *addr = duk_require_pointer(ctx,0);
    
    if (addr->sa_family == AF_INET){
        return como_sock_inet_ntop4(ctx);
    }
    
    else if (addr->sa_family == AF_INET6){
        return como_sock_inet_ntop6(ctx);
    }
    
    else {
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
}

/*=============================================================================
  create socket
 ============================================================================*/
COMO_METHOD(como_sock_socket) {
    int af       = duk_require_int(ctx, 0);
    int type     = duk_require_int(ctx, 1);
    int protocol = duk_require_int(ctx,2);
    
    SOCKET sock = socket(af, type, protocol);
    if (sock == INVALID_SOCKET){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_int(ctx, sock);
    return 1;
}

/*=============================================================================
  bind
 ============================================================================*/
COMO_METHOD(como_sock_bind) {
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
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  listen for connections on a socket, called from javascript land, with 2
  arguments a pointer to a packed ip address struct and a backlog number
  of connections socket can listen to

   * @param {Pointer} pointer to a packed ip address
   * @param {Number} backlog number of max connections

 ============================================================================*/
COMO_METHOD(como_sock_listen) {
    SOCKET s    = duk_require_int(ctx, 0);
    int backlog = duk_require_int(ctx, 1);
    
    if (listen(s, backlog) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  initiate a connection on a socket, called from javascript land, with 2
  arguments a socket handle/fd and packed ip address pointer
  
  * @param {SOCK} fd/handle of the socket to be connected 
  * @param {Pointer} packed ip address

 ============================================================================*/
COMO_METHOD(como_sock_connect) {
    SOCKET s              = duk_require_int(ctx, 0);
    struct sockaddr *addr = duk_require_pointer(ctx, 1);
    
    unsigned int addrlen;
    if (addr->sa_family == AF_INET){
        addrlen = sizeof(struct sockaddr_in);
    } else if (addr->sa_family == AF_INET6){
        addrlen = sizeof(struct sockaddr_in6);
    } else {
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }
    
    if (connect(s, addr, addrlen) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  accept a connection on a socket , called from javascript land,
  with 1 argument, a socket fd/handle
  
  * @param {SOCKET} socket fd/handle 
 ============================================================================*/
COMO_METHOD(como_sock_accept) {
    SOCKET s = duk_require_int(ctx, 0);
    
    SOCKET peer = accept(s, NULL, NULL);
    if (peer ==  INVALID_SOCKET){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    duk_push_int(ctx, peer);
    return 1;
}

int como_core_nonblock(int fd, int set) {
    int r;
    #ifdef _WIN32
        u_long iMode = set;
        r = ioctlsocket(fd, FIONBIO, &iMode);
        if ( r != 0 ){
            return WSAGetLastError();
        }
    #else
        int flags;
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

/*=============================================================================
  mark open socket as non-blocking or blocking , called from javascript 
  land, accepts 2 argument, a socket fd/handle, a set (1|0)
  
  * @param {SOCKET} socket fd/handle
  * @param {SET} (0|1) 1 for marking socket as non blocking, 0 for blocking

 ============================================================================*/
COMO_METHOD(como_sock_nonblock) {
    SOCKET s = duk_require_int(ctx, 0);
    int set  = duk_require_int(ctx, 1);
    int ret  = como_core_nonblock(s,set);
    if (ret) {
        COMO_SET_ERRNO_AND_RETURN(ctx, ret);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  recv
 ============================================================================*/
//FIXME: this is really ugly
//use push_fixed_buffer instead
COMO_METHOD(como_sock_recv) {
    SOCKET fd  = duk_require_int(ctx, 0);
    size_t len = (size_t)duk_require_int(ctx, 1);
    int flags  = duk_get_int(ctx, 2); /* default to zero */

    char *buf = malloc(sizeof(char)*len);
    if (buf == NULL){
        assert(0 && "can't allocate memory for recieveing data");
    }

    size_t nread;
    do {
        nread = recv(fd, buf, len, flags);
    } while (nread < 0 && COMO_GET_LAST_WSA_ERROR == SOCKEINTR);

    if (nread == -1){
        free(buf);
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    } else if (nread == 0){
        free(buf);
        COMO_SET_ERRNO_AND_RETURN(ctx, COMOEOF);
    }
    
    duk_push_lstring(ctx, buf, nread);
    free(buf);
    return 1;
}

/*=============================================================================
  read into buffer

  ex:
  var readBytes = socket.readIntoBuffer(fd, buffer, recv flags);
 ============================================================================*/
COMO_METHOD(como_sock_readIntoBuffer) {
    SOCKET fd  = duk_require_int(ctx, 0);

    size_t len, nread;
    char *buf = duk_require_buffer(ctx, 1, &len);
    int flags  = duk_get_int(ctx, 2); /* default to zero */

    do {
        nread = recv(fd, buf, len, flags);
    } while (nread < 0 && COMO_GET_LAST_WSA_ERROR == SOCKEINTR);

    if (nread == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    } else if (nread == 0){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMOEOF);
    }

    duk_push_number(ctx, (double)nread);
    return 1;
}

/*=============================================================================
  send (string/buffer)
 ============================================================================*/
COMO_METHOD(como_sock_send) {
    SOCKET fd       = duk_require_int(ctx, 0);
    const char *buf;
    size_t length;

    if (duk_get_type(ctx, 1) == DUK_TYPE_BUFFER){
        buf = duk_get_buffer(ctx, 1, &length);
    } else {
        buf = duk_require_lstring(ctx, 1, &length);
    }

    size_t len      = (size_t)duk_require_int(ctx, 2);
    int flags       = duk_get_int(ctx, 3);

    /* do not allow to exceed string size when sending */
    if (len > length) len = length;
    
    size_t n;
    do {
        n = send(fd, buf, len, flags);
    } while (n == -1 && errno == SOCKEINTR);

    if (n == -1){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }

    duk_push_number(ctx, (double)n);
    return 1;
}

/*=============================================================================
  Return a pair of connected local sockets, will be emulated on windows to
  to use TCP sockets
 ============================================================================*/

COMO_METHOD(como_sock_socketpair) {
    int dummy = duk_get_int(ctx, 0);
    SOCKET sockets[2];
    if (dumb_socketpair(sockets, dummy) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }

    duk_idx_t arr_idx = duk_push_array(ctx);
    duk_push_int(ctx, sockets[0]);
    duk_put_prop_index(ctx, arr_idx, 0);
    duk_push_int(ctx, sockets[1]);
    duk_put_prop_index(ctx, arr_idx, 1);
    return 1;
}

/*=============================================================================
  shutdown
 ============================================================================*/
COMO_METHOD(como_sock_shutdown) {
    SOCKET s = duk_require_int(ctx, 0);
    int how  = duk_get_int(ctx, 1);

    if (shutdown(s, how) == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  close
 ============================================================================*/
COMO_METHOD(como_sock_close) {

    if (!duk_is_number(ctx, 0)){
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    SOCKET s = duk_get_int(ctx, 0);
    
    int ret;
    #ifdef _WIN32
        ret = closesocket(s);
    #else
        ret = close(s);
    #endif
    
    if (ret == SOCKET_ERROR){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    
    duk_push_true(ctx);
    return 1;
}

int hostname_to_ip(const char *hostname , const char *protocol, char *ip) {
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
 
    if ( (rv = getaddrinfo( hostname , protocol, &hints , &servinfo)) != 0) {
        return rv;
    }

    /* loop through all the results and connect to the first we can */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }

    freeaddrinfo(servinfo); /* all done with this structure */
    return 0;
}

COMO_METHOD(como_host_to_ip) {
    const char *hostname = duk_require_string(ctx, 0);
    const char *protocol = duk_require_string(ctx, 1);
    char ip[100];
    
    int er = hostname_to_ip(hostname , protocol, ip);

    if (er){ COMO_SET_ERRNO_AND_RETURN(ctx, er); }
    duk_push_string(ctx, ip);
    return 1;
}



/*=============================================================================
  socket export functions list
 ============================================================================*/
static const duk_function_list_entry como_socket_funcs[] = {
    { "pton4"          , como_sock_inet_pton4, 2 },
    { "pton6"          , como_sock_inet_pton6, 2 },
    { "pton"           , como_sock_inet_pton, 2 },
    { "ntop4"          , como_sock_inet_ntop4, 1 },
    { "ntop6"          , como_sock_inet_ntop6, 1 },
    { "ntop"           , como_sock_inet_ntop, 1 },
    {"host_to_ip"      , como_host_to_ip, 2},
    { "socket"         , como_sock_socket, 3 },
    { "setsockopt"     , como_sock_setsockopt, 4 },
    { "getsockopt"     , como_sock_getsockopt, 3 },
    { "bind"           , como_sock_bind, 2 },
    { "listen"         , como_sock_listen, 2 },
    { "connect"        , como_sock_connect, 3 },
    { "accept"         , como_sock_accept, 1 },
    { "nonblock"       , como_sock_nonblock, 2 },
    { "recv"           , como_sock_recv, 3},
    {"readIntoBuffer"  , como_sock_readIntoBuffer, 3},
    { "send"           , como_sock_send, 4},
    { "shutdown"       , como_sock_shutdown,2 },
    { "close"          , como_sock_close, 1 },
    { "getprotobyname" , como_sock_getprotobyname, 1},
    { "getpeername"    , como_sock_getpeername, 1},
    { "getsockname"    , como_sock_getsockname, 1},
    { "isIP"           , como_sock_isIP, 1},
    { "family"         , como_sock_get_family, 1 },
    { "addr_info"      , como_sock_address_info, 1 },
    { "socketpair"     , como_sock_socketpair, 0},
    { NULL         , NULL, 0 }
};

/*=============================================================================
  socket export constants
 ============================================================================*/
static const duk_number_list_entry como_socket_constants[] = {
    { "IPPROTO_IP"  , IPPROTO_IP},
    /* socket domain constants */
    { "AF_INET"     , AF_INET },
    { "AF_INET6"    , AF_INET6 },
    { "AF_UNIX"     , AF_UNIX },
    { "AF_UNSPEC"   , AF_UNSPEC },
    
    /* socket type constants */
    { "SOCK_STREAM"    , SOCK_STREAM },
    { "SOCK_SEQPACKET" , SOCK_SEQPACKET },
    { "SOCK_RAW"       , SOCK_RAW },
    { "SOCK_DGRAM"     , SOCK_DGRAM },
    { "SOCK_RDM"       , SOCK_RDM },
    #ifdef SOCK_NONBLOCK
    { "SOCK_NONBLOCK" , SOCK_NONBLOCK },
    #endif
    #ifdef SOCK_CLOEXEC
    { "SOCK_CLOEXEC" , SOCK_CLOEXEC },
    #endif
    
    /* socket options */
    //SOL_SOCKET level
    { "SOL_SOCKET"   , SOL_SOCKET },
    { "SO_DEBUG"     , SO_DEBUG },
    { "SO_BROADCAST" , SO_BROADCAST },
    { "SO_KEEPALIVE" , SO_KEEPALIVE },
    { "SO_LINGER"    , SO_LINGER },
    { "SO_SNDBUF"    , SO_SNDBUF },
    { "SO_RCVBUF"    , SO_RCVBUF },
    { "SO_RCVTIMEO"  , SO_RCVTIMEO },
    { "SO_SNDTIMEO"  , SO_SNDTIMEO },
    { "SO_SNDLOWAT"  , SO_SNDLOWAT },
    { "SO_RCVLOWAT"  , SO_RCVLOWAT },
    { "SO_REUSEADDR" , SO_REUSEADDR},
    { "SO_ERROR"     , SO_ERROR},
    { "SO_TYPE"      , SO_TYPE},
    
    #ifdef SO_NOSIGPIPE
    { "SO_NOSIGPIPE" , SO_NOSIGPIPE},
    #endif
    
    #ifdef SO_EXCLUSIVEADDRUSE
    { "SO_EXCLUSIVEADDRUSE", SO_EXCLUSIVEADDRUSE },
    #endif
    
    #ifdef SO_NOSIGPIPE
    { "SO_NOSIGPIPE", SO_NOSIGPIPE },
    #endif
    
    #ifdef MSG_NOSIGNAL
    { "MSG_NOSIGNAL", MSG_NOSIGNAL },
    #endif

    //IPPROTO_TCP level
    { "IPPROTO_TCP" , IPPROTO_TCP},
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

    { "INADDR_ANY"       , INADDR_ANY },
    { "INADDR_LOOPBACK"  , INADDR_LOOPBACK },
    
    { NULL, 0 }
};

/** 
  * socket function initiation
******************************************************************************/
static int init_binding_socket(duk_context *ctx) {
    
    #ifdef _WIN32
        WSADATA wsaData = {0};
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            assert(0 && "couldn't initiate sockets");
        }
    #endif

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_socket_funcs);
    duk_put_number_list(ctx, -1, como_socket_constants);
    return 1;
}
