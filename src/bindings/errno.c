#if !defined(EADDRINUSE) && defined(_WIN32)
# define EADDRINUSE WSAEADDRINUSE
#endif

#if !defined(ENOTSOCK) && defined(_WIN32)
# define ENOTSOCK WSAENOTSOCK
#endif

#if !defined(ECONNRESET) && defined(_WIN32)
# define ECONNRESET WSAECONNRESET
#endif

#if !defined(EWOULDBLOCK) && defined(_WIN32)
# define EWOULDBLOCK  WSAEWOULDBLOCK
#endif

#if !defined(ECONNABORTED) && defined(_WIN32)
# define ECONNABORTED  WSAECONNABORTED
#endif

#if !defined(EAFNOSUPPORT) && defined(_WIN32)
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif

#if !defined(EACCES) && defined(_WIN32)
#define EACCES WSAEACCES
#endif

#if !defined(EMFILE) && defined(_WIN32)
#define EMFILE WSAEMFILE
#endif

#if !defined(ESOCKTNOSUPPORT) && defined(_WIN32)
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#endif

#if !defined(EPROTONOSUPPORT) && defined(_WIN32)
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#endif

#if !defined(EADDRNOTAVAIL) && defined(_WIN32)
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#endif

#if !defined(ECONNREFUSED) && defined(_WIN32)
#define ECONNREFUSED WSAECONNREFUSED
#endif

static const int _errno_toString(duk_context *ctx) {
    int ERRNO = duk_get_int(ctx,0);
    duk_push_string(ctx, strerror(ERRNO));
    return 1;
}

static const duk_function_list_entry errno_funcs[] = {
    { "toString", _errno_toString, 1 },
    { NULL, NULL, 0 }
};

static const duk_number_list_entry errno_constants[] = {
    { "EADDRINUSE"         , EADDRINUSE },
    { "EACCES"             , EACCES },
    { "EMFILE"             , EMFILE  },
    { "EINVAL"             , EINVAL   },
    { "ENOTSOCK"           , ENOTSOCK },
    { "EWOULDBLOCK"        , EWOULDBLOCK },
    { "EAGAIN"             , EAGAIN },
    { "EBADF"              , EBADF },
    { "ECONNABORTED"       , ECONNABORTED },
    { "ECONNRESET"         , ECONNRESET},
    { "EAFNOSUPPORT"       , EAFNOSUPPORT},
    { "ESOCKTNOSUPPORT"    , ESOCKTNOSUPPORT},
    { "EPROTONOSUPPORT"    , EPROTONOSUPPORT},
    { "EADDRNOTAVAIL"      , EADDRNOTAVAIL},
    { "ECONNREFUSED"       , ECONNREFUSED},
    
    #ifdef WSAEINVAL
    { "WSAEINVAL"          , WSAEINVAL},
    #endif
    { NULL, 0 }
};

static const int init_binding_errno(duk_context *ctx) {
    
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, errno_funcs);
    duk_put_number_list(ctx, -1, errno_constants);
    return 1;
}
