#include "como_errno.h"

COMO_METHOD(como_errno_toString) {
    int ERRNO = duk_get_int(ctx,0);
    duk_push_string(ctx, strerror(ERRNO));
    return 1;
}

static const duk_function_list_entry errno_funcs[] = {
    { "toString", como_errno_toString, 1 },
    { NULL, NULL, 0 }
};

static const duk_number_list_entry errno_constants[] = {
    { "EADDRINUSE"         , EADDRINUSE },
    { "EACCES"             , EACCES },
    { "ENFILE"             , ENFILE  },
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
    { "EINPROGRESS"        , EINPROGRESS},
    { "EALREADY"           , EALREADY},
    { "EISCONN"            , EISCONN},
    { "ENOBUFS"            , ENOBUFS},
    { "EINTR"              , EINTR},
    { "EPIPE"              , EPIPE},
    { "EOF"                , COMOEOF},
    { "EBUSY"              , EBUSY},
    { "ENOTCONN"           , ENOTCONN},
    #ifdef WSAEINVAL
    { "WSAEINVAL"          , WSAEINVAL},
    #endif
    { NULL, 0 }
};

static int init_binding_errno(duk_context *ctx) {
    
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, errno_funcs);
    duk_put_number_list(ctx, -1, errno_constants);
    return 1;
}
