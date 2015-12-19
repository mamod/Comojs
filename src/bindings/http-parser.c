#include "http/http_parser.h"

typedef struct comoHttpParser {
    duk_context *ctx;
    void *self;
} comoHttpParser;

#define _JS_CB(FOR) \
do { \
    comoHttpParser *cp = p->data;\
    duk_context *ctx = cp->ctx;\
    duk_push_heapptr(ctx, cp->self);\
    duk_get_prop_string(ctx, -1, FOR);\
    duk_push_heapptr(ctx, cp->self);\
    if (len > 0){\
        duk_push_lstring(ctx, buf, len); \
    } else {\
        duk_push_lstring(ctx, "", 0); \
    }\
    duk_call_method(ctx, 1);\
    duk_idx_t idx_top = duk_get_top_index(ctx);\
    duk_pop_n(ctx, idx_top + 1);\
    return duk_get_int(ctx, -1);\
} while(0)

/* callback functions */
static int __url_cb (http_parser *p, const char *buf, size_t len) {
    _JS_CB("on_url");
}

static int __status_cb (http_parser *p, const char *buf, size_t len) {
    _JS_CB("on_status");
}

static int __header_field_cb (http_parser *p, const char *buf, size_t len) {
    _JS_CB("on_header_field");
}

static int __header_value_cb (http_parser *p, const char *buf, size_t len) {
    _JS_CB("on_header_value");
}

static int __body_cb (http_parser *p, const char *buf, size_t len) {
    _JS_CB("on_body");
}

/* notify functions */
static int __headers_complete_cb (http_parser *p) {
    const char *buf = "";
    int len = 0;
    _JS_CB("on_headers_complete");
}

static int __message_complete_cb (http_parser *p) {
    const char *buf = "";
    int len = 0;
    _JS_CB("on_message_complete");
}

static int __message_begin_cb (http_parser *p) {
    const char *buf = "";
    int len = 0;
    _JS_CB("on_message_begin");
}

static http_parser_settings settings_ = {
    .on_message_begin = __message_begin_cb
    ,.on_header_field = __header_field_cb
    ,.on_header_value = __header_value_cb
    ,.on_url = __url_cb
    ,.on_status = __status_cb
    ,.on_body = __body_cb
    ,.on_headers_complete = __headers_complete_cb
    ,.on_message_complete = __message_complete_cb
};

COMO_METHOD(como_http_parser_init) {
    enum http_parser_type type = duk_require_int(ctx, 0);
    void *self = duk_to_pointer(ctx, 1);

    http_parser *p = malloc(sizeof(*p));
    if (p == NULL){
        COMO_SET_ERRNO_AND_RETURN(ctx, ENOMEM);
    }
    
    comoHttpParser *cp = malloc(sizeof(*cp));
    cp->ctx = ctx;
    cp->self = self;
    p->data = cp;
    http_parser_init(p, type);
    duk_push_pointer(ctx, p);
    return 1;
}

size_t _http_parser_parse_thread (http_parser *p, const char *str, size_t len) {
    size_t nparsed = http_parser_execute(p, &settings_, str, len);
    return nparsed;
}

COMO_METHOD(como_http_parser_parse) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    const char *str = duk_get_string(ctx, 1);
    size_t len      = duk_get_int(ctx, 2);
    size_t nparsed  = http_parser_execute(p, &settings_, str, len);
    duk_push_int(ctx, (size_t)nparsed);
    return 1;
}

COMO_METHOD(como_http_parser_reinitialize) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    enum http_parser_type type = duk_require_int(ctx, 1);
    http_parser_init(p, type);
    duk_push_true(ctx);
    return 1;
}

COMO_METHOD(como_http_should_keep_alive) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    int ret = http_should_keep_alive(p);
    if (ret == 0){
        duk_push_false(ctx);
    } else {
        duk_push_true(ctx);
    }
    return 1;
}

COMO_METHOD(como_http_minor) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    int ret = p->http_minor;
    duk_push_int(ctx, ret);
    return 1;
}

COMO_METHOD(como_http_major) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    int ret = p->http_major;
    duk_push_int(ctx, ret);
    return 1;
}

COMO_METHOD(como_http_method) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    int ret = p->method;
    duk_push_int(ctx, ret);
    return 1;
}

COMO_METHOD(como_http_upgrade) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    int ret = p->upgrade;
    if (ret == 0){
        duk_push_false(ctx);
    } else {
        duk_push_true(ctx);
    }
    return 1;
}

COMO_METHOD(como_http_parser_destroy) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    comoHttpParser *data = p->data;
    free(data);
    free(p);
    return 1;
}

static const duk_number_list_entry como_http_parser_constants[] = {
    {"HTTP_REQUEST"  , HTTP_REQUEST},
    {"HTTP_RESPONSE" , HTTP_RESPONSE},
    {"HTTP_BOTH"     , HTTP_BOTH},
    //request methods
    {"HTTP_GET"      , HTTP_GET},
    {"HTTP_PUT"      , HTTP_PUT},
    {"HTTP_POST"     , HTTP_POST},
    {"HTTP_DELETE"   , HTTP_DELETE},
    {"HTTP_MSEARCH"  , HTTP_MSEARCH},
    {"HTTP_PATCH"    , HTTP_PATCH},
    {"HTTP_CONNECT"  , HTTP_CONNECT},
    { NULL, 0 }
};

static const duk_function_list_entry como_http_parser_funcs[] = {
    { "init"                    , como_http_parser_init, 2 },
    { "parse"                   , como_http_parser_parse, 3 },
    { "reinitialize"            , como_http_parser_reinitialize, 2 },
    { "http_should_keep_alive"  , como_http_should_keep_alive, 1 },
    { "http_minor"              , como_http_minor, 1 },
    { "http_major"              , como_http_major, 1 },
    { "http_upgrade"            , como_http_upgrade, 1 },
    { "http_method"             , como_http_method, 1 },
    { "destroy"                 , como_http_parser_destroy, 1 },
    { NULL                      , NULL, 0 }
};

static int init_binding_parser(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_http_parser_funcs);
    duk_put_number_list(ctx, -1, como_http_parser_constants);
    return 1;
}
