#ifndef COMO_NO_TLS

#include "mbedtls/config.h"
#include "mbedtls/platform.h"

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

#include "como_errno.h"

#define MBEDTLS_SSL_CACHE_C
#define DEBUG_LEVEL 0

#if defined(MBEDTLS_SSL_CACHE_C)
    #include "mbedtls/ssl_cache.h"
#endif

typedef struct {
    mbedtls_entropy_context *entropy;
    mbedtls_ctr_drbg_context *ctr_drbg;
    mbedtls_ssl_context *ssl;
    mbedtls_ssl_config *conf;
    mbedtls_x509_crt *srvcert;
    mbedtls_pk_context *pkey;
    void *read;
    void *write;
} comoTLS;

typedef struct {
    comoTLS *tls;
    mbedtls_ssl_context *ssl;
    duk_context *ctx;
    int fd;
} comoTlsContext;


/* copied from mbedtls/ssl_cache.c */
void como_mbedtls_ssl_cache_init( mbedtls_ssl_cache_context *cache, int cache_time ){
    memset( cache, 0, sizeof( mbedtls_ssl_cache_context ) );
    cache->timeout     = cache_time;
    cache->max_entries = 25;

    #if defined(MBEDTLS_THREADING_C)
        mbedtls_mutex_init( &cache->mutex );
    #endif
}

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str ){
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

int como_tls_context_send( void *data, const unsigned char *buf, size_t len ){
    comoTlsContext *tlsContext = data;
    int ret = -1;
    comoTLS *tls = tlsContext->tls;
    int fd = tlsContext->fd;
    
    /* if tls initiation provided it's own write callback */
    if (tls->write != NULL){
        duk_context *ctx = tlsContext->ctx;
        duk_push_heapptr(ctx, tls->write);
        duk_push_int(ctx, fd);
        
        char *buffer = duk_push_fixed_buffer(ctx, len);
        memcpy(buffer, (const char *)buf, len);

        duk_push_number(ctx, len);

        duk_call(ctx, 3);
        if (duk_get_type(ctx, -1) == DUK_TYPE_NUMBER){
            ret = duk_get_int(ctx, -1);
        }

        duk_pop(ctx);
        return ret;
    }

    #ifdef MSG_NOSIGNAL
        int flags = MSG_NOSIGNAL;
    #else
        int flags = 0;
    #endif
    ret = send( fd, (char *)buf, len, flags );
    return ret;
}

int como_tls_context_recv( void *data, unsigned char *buf, size_t len ) {
    comoTlsContext *tlsContext = data;
    comoTLS *tls = tlsContext->tls;
    int fd = tlsContext->fd;
    int ret = -1;

    /* if tls initiation provided it's own read callback */
    if (tls->read != NULL){
        duk_context *ctx = tlsContext->ctx;
        duk_push_heapptr(ctx, tls->read);
        duk_push_int(ctx, fd);

        char *buffer = duk_push_fixed_buffer(ctx, len);

        duk_push_number(ctx, len);
        duk_call(ctx, 3);
        
        
        if (duk_get_type(ctx, -1) == DUK_TYPE_NUMBER){
            ret = duk_get_int(ctx, -1);
            if (ret > 0){
                memcpy((char *)buf, (const char *)buffer, ret);
            }
        }

        duk_pop(ctx);
        return ret;
    }

    #ifdef MSG_NOSIGNAL
        int flags = MSG_NOSIGNAL;
    #else
        int flags = 0;
    #endif
    ret = recv( fd, (char *)buf, len, flags );
    return ret;
}

/*=============================================================================
  internal usage
  free tlsContext
 ============================================================================*/
void como_free_tlsContext(comoTlsContext *tlsContext) {
    mbedtls_ssl_free( tlsContext->ssl );
    free( tlsContext->ssl );
    free( tlsContext );
}

/*=============================================================================
  context handshake
  issue a secure handshake
  
  on failure returns null, 1 otherwise

  ex:
  if (!context.handshake()){
    //you may need to close this connection
    context.close();
    socket.close(fd);
  }

  //resume once handshake is good
  context.read();
  ...
 ============================================================================*/
COMO_METHOD(como_tls_context_handshake) {

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""tlsContext");
    comoTlsContext *tlsContext = duk_get_pointer(ctx, -1);
    duk_pop_2(ctx);

    if ( tlsContext == NULL ){
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    int ret;
    ret = mbedtls_ssl_handshake( tlsContext->ssl );
    if ( ret == -1 ){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }
    duk_push_int(ctx, ret);
    return 1;
}

/*=============================================================================
  tls context read
  perform a secure read from the underlying socket

  returns read string on success
  return null on failure and sets process.errno

  ex:

  //handshae ... ok
  var data = context.read();
 ============================================================================*/
#define COMO_TLS_READ_LENGTH 8 * 1024
COMO_METHOD(como_tls_context_read) {
    
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""tlsContext");
    comoTlsContext *tlsContext = duk_get_pointer(ctx, -1);
    duk_pop_2(ctx);

    if ( tlsContext == NULL ){
        COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    }

    unsigned char buf[COMO_TLS_READ_LENGTH];

    int nread;
    do {
        nread = mbedtls_ssl_read( tlsContext->ssl, buf, COMO_TLS_READ_LENGTH );
    } while (nread < 0 && COMO_GET_LAST_WSA_ERROR == SOCKEINTR);

    if (nread < 0){
        COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    }

    duk_push_lstring(ctx, (char *)buf, nread);

    return 1;
}

/*=============================================================================
  tls context write
  perform a secure data write to the underlying connected socket

  requires 1 argument
  string to write

  ex:
  context.write("some string");
 ============================================================================*/
//FIXME : dynamic allocation?!
COMO_METHOD(como_tls_context_write) {

    // size_t length;
    // const char *str = duk_require_lstring(ctx, 0, &length);

    // duk_push_this(ctx);
    // duk_get_prop_string(ctx, -1, "\xff""tlsContext");
    // comoTlsContext *tlsContext = duk_get_pointer(ctx, -1);
    // duk_pop_2(ctx);

    // if ( tlsContext == NULL ){
    //     COMO_SET_ERRNO_AND_RETURN(ctx, EINVAL);
    // }

    // unsigned char buf[length];
    // size_t len = sprintf( (char *) buf, str,
    //                mbedtls_ssl_get_ciphersuite( tlsContext->ssl ) );

    // int written;
    // do {
    //     written = mbedtls_ssl_write( tlsContext->ssl, buf, len );
    // } while (written <= 0 && COMO_GET_LAST_WSA_ERROR == SOCKEINTR);
    // if (written < 0){
    //     COMO_SET_ERRNO_AND_RETURN(ctx, COMO_GET_LAST_WSA_ERROR);
    // }

    // duk_push_int(ctx, written);
    return 1;
}

/*=============================================================================
  close tls context
  
  closing a context is required to free resources

  ex:
  //once you're done with the current connection
  context.close();

  Please note that closing a context doesn't close the underlying associated
  socket fd, so you need to close that fd
 ============================================================================*/
COMO_METHOD(como_tls_context_close) {
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""tlsContext");
    comoTlsContext *tlsContext = duk_get_pointer(ctx, -1);

    /* already closed */
    if(tlsContext == NULL){
        duk_pop_2(ctx);
        return 1;
    };

    como_free_tlsContext(tlsContext);

    duk_pop(ctx);
    duk_push_pointer(ctx, NULL);
    duk_put_prop_string(ctx, -2, "\xff""tlsContext");
    duk_pop(ctx);

    return 1;
}

static const duk_function_list_entry como_tls_context_func[] = {
    { "handshake", como_tls_context_handshake,   0},
    { "read", como_tls_context_read,             0},
    { "write", como_tls_context_write,           1},
    { "close", como_tls_context_close,           0},
    { NULL, NULL,                                0}
};

/*=============================================================================
  init a tls context
  requires 1 argument
  1- connected client socket fd
  
  returns an object on success, null on failure 
  {
    handshake : function(){},
    read      : function(){},
    write     : function(){},
    close     : function(){}
  }

  ex: 
  var context = ssl.context(fd);
  //issue a handshake process
  if (!context.handshake()){
    //handshake failed
  }
 ============================================================================*/
COMO_METHOD(como_tls_context) {
    int fd = duk_require_int(ctx, 0);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""tlsPointer");
    comoTLS *tls = duk_require_pointer(ctx, -1);
    duk_pop_2(ctx);

    int ret;

    mbedtls_ssl_context *ssl = malloc(sizeof(*ssl));
    /* Make sure memory references are valid */
    mbedtls_ssl_init( ssl );
    if( ( ret = mbedtls_ssl_setup( ssl, tls->conf ) ) != 0 ){
        goto reset;
    }

    comoTlsContext *tlsContext = malloc(sizeof(*tlsContext));
    tlsContext->ssl    = ssl;
    tlsContext->ctx    = ctx;
    tlsContext->fd     = fd;
    tlsContext->tls    = tls;

    mbedtls_ssl_set_bio( ssl, tlsContext, como_tls_context_send, como_tls_context_recv, NULL );

    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_tls_context_func);
    duk_push_pointer(ctx, tlsContext);
    duk_put_prop_string(ctx, -2, "\xff""tlsContext");

    duk_push_int(ctx, fd);
    duk_put_prop_string(ctx, -2, "fd");

    return 1;

    reset:
        mbedtls_ssl_free( ssl );
        duk_push_null(ctx);
        return 1;
}

static const duk_function_list_entry como_tls_init_func[] = {
    { "context", como_tls_context,            1},
    { NULL, NULL,                             0}
};

duk_ret_t como_tls_finalizer(duk_context *ctx) {
    /* Object being finalized is at stack index 0. */
    assert(0 && "should not be called for now");
    return 0;
}


/*=============================================================================
  init a tls connection
  requires 2 arguments 
  1- full path to certificate
  2- full path to key
  
  returns an object { context : function(){} }

  ex: 
  var ssl = tls.init("cert.pem", "key.pem");
  //later on initiate a new context for socket fd
  var context = ssl.context(fd);
 ============================================================================*/
COMO_METHOD(como_tls_init) {
    const char *cert = duk_require_string(ctx, 0);
    const char *key  = duk_require_string(ctx, 1);
    
    int cache_time = 0;
    void *read_callback = NULL;
    void *write_callback = NULL;

    /*
        getting options
        read : function{},
        write : function(),
        cache : 3600
     */
    if (duk_get_type(ctx, 2) == DUK_TYPE_OBJECT){
        
        duk_get_prop_string(ctx, -1, "read");
        if ( duk_is_function(ctx, -1) ){
            read_callback = duk_require_heapptr(ctx, -1);
        }
        
        duk_get_prop_string(ctx, -2, "write");
        if ( duk_is_function(ctx, -1) ){
            write_callback = duk_require_heapptr(ctx, -1);
        }

        duk_get_prop_string(ctx, -3, "cache");
        if ( duk_is_number(ctx, -1) ){
            cache_time = duk_get_int(ctx, -1);
        }
        
        duk_pop_n(ctx, 3);
    }

    int ret;
    const char *pers = "ssl_server";

    mbedtls_entropy_context *entropy = malloc(sizeof(*entropy));
    mbedtls_ctr_drbg_context *ctr_drbg = malloc(sizeof(*ctr_drbg));
    mbedtls_ssl_context *ssl = malloc(sizeof(*ssl));
    mbedtls_ssl_config *conf = malloc(sizeof(*conf));
    mbedtls_x509_crt *srvcert = malloc(sizeof(*srvcert));
    mbedtls_pk_context *pkey = malloc(sizeof(*pkey));
    
    #if defined(MBEDTLS_SSL_CACHE_C)
        mbedtls_ssl_cache_context *cache = malloc(sizeof(*cache));
    #endif

    mbedtls_ssl_init( ssl );
    mbedtls_ssl_config_init( conf );
    
    #if defined(MBEDTLS_SSL_CACHE_C)
        como_mbedtls_ssl_cache_init( cache, cache_time );
    #endif
    
    mbedtls_x509_crt_init( srvcert );
    mbedtls_pk_init( pkey );
    mbedtls_entropy_init( entropy );
    mbedtls_ctr_drbg_init( ctr_drbg );

    #if defined(MBEDTLS_DEBUG_C)
        mbedtls_debug_set_threshold( DEBUG_LEVEL );
    #endif

    /* 1. Load the certificates and private RSA key */
    ret = mbedtls_x509_crt_parse_file( srvcert, cert);
    if( ret != 0 ) {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    // ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_cas_pem,
    //                       mbedtls_test_cas_pem_len );
    // if( ret != 0 )
    // {
    //     mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
    //     goto exit;
    // }

    ret =  mbedtls_pk_parse_keyfile( pkey, key, NULL);
    if( ret != 0 ) {
        mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret );
        goto exit;
    }

    /* 3. Seed the RNG */
    if( ( ret = mbedtls_ctr_drbg_seed( ctr_drbg, mbedtls_entropy_func, entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 ){
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    /* 4. Setup stuff */
    if( ( ret = mbedtls_ssl_config_defaults( conf,
                    MBEDTLS_SSL_IS_SERVER,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ){
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, ctr_drbg );
    mbedtls_ssl_conf_dbg( conf, my_debug, stdout );

    #if defined(MBEDTLS_SSL_CACHE_C)
        mbedtls_ssl_conf_session_cache( conf, cache,
                                   mbedtls_ssl_cache_get,
                                   mbedtls_ssl_cache_set );
    #endif

    mbedtls_ssl_conf_ca_chain( conf, srvcert->next, NULL );
    if( ( ret = mbedtls_ssl_conf_own_cert( conf, srvcert, pkey ) ) != 0 ){
        mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_setup( ssl, conf ) ) != 0 ){
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        goto exit;
    }

    comoTLS *tls = malloc(sizeof(*tls));
    
    tls->ssl = ssl;
    tls->conf = conf;
    tls->entropy = entropy;
    tls->srvcert = srvcert;
    tls->ctr_drbg = ctr_drbg;
    tls->pkey = pkey;
    tls->read = read_callback;
    tls->write = write_callback;
    
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_tls_init_func);
    duk_push_pointer(ctx, tls);
    duk_put_prop_string(ctx, -2, "\xff""tlsPointer");
    
    duk_push_c_function(ctx, como_tls_finalizer, 1 /*nargs*/);
    duk_set_finalizer(ctx, -2);
    
    return 1;

    exit:
    duk_push_null(ctx);
    return 1;
}
#else
COMO_METHOD(como_tls_init) {
    assert(0 && "como built without tls support");
    return 1;
}
#endif //COMO_NO_TLS

static const duk_function_list_entry como_tls_funcs[] = {
    { "init",               como_tls_init,          3 },
    { NULL,                 NULL,                   0 }
};

static int init_binding_tls(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_tls_funcs);
    return 1;
}
