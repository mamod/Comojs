#include "../include/http/http_parser.h"

typedef struct comoHttpParser {
    duk_context *ctx;
    void *self;
    void *cb;
} comoHttpParser;

#define _JS_CB(FOR) \
do { \
    comoHttpParser *cp = p->data;\
    duk_context *ctx = cp->ctx;\
    duk_push_object_pointer(ctx, cp->self);\
    duk_get_prop_string(ctx, -1, FOR);\
    duk_push_object_pointer(ctx, cp->self);\
    if (len > 0){\
        duk_push_lstring(ctx, buf, len); \
    } else {\
        duk_push_lstring(ctx, "", 0); \
    }\
    duk_call_method(ctx, 1);\
    duk_pop_2(ctx);\
    /*dump_stack(ctx, "PARSER");*/ \
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
    char *buf = "";
    int len = 0;
    _JS_CB("on_headers_complete");
}

static int __message_complete_cb (http_parser *p) {
    char *buf = "";
    int len = 0;
    _JS_CB("on_message_complete");
}

static int __message_begin_cb (http_parser *p) {
    char *buf = "";
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

static const int _http_parser_init(duk_context *ctx) {
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

static const int _http_parser_parse(duk_context *ctx) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    const char *str = duk_get_string(ctx, 1);
    size_t len = duk_get_int(ctx, 2);
    size_t nparsed = http_parser_execute(p, &settings_, str, len);
    duk_push_int(ctx, (size_t)nparsed);
    return 1;
}

static const int _http_parser_reinitialize(duk_context *ctx) {
    http_parser *p  = duk_require_pointer(ctx, 0);
    enum http_parser_type type = duk_require_int(ctx, 1);
    http_parser_init(p, type);
    duk_push_true(ctx);
    return 1;
}

static const duk_number_list_entry como_http_parser_constants[] = {
    {"HTTP_REQUEST"  , HTTP_REQUEST},
    {"HTTP_RESPONSE" , HTTP_RESPONSE},
    {"HTTP_BOTH"     , HTTP_BOTH},
    { NULL, 0 }
};

static const duk_function_list_entry como_http_parser_funcs[] = {
    { "init"          , _http_parser_init, 2 },
    { "parse"         , _http_parser_parse, 3 },
    { "reinitialize"  , _http_parser_reinitialize, 2 },
    { NULL            , NULL, 0 }
};

static const int init_binding_parser(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_http_parser_funcs);
    duk_put_number_list(ctx, -1, como_http_parser_constants);
    return 1;
}
