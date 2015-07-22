#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"

COMO_METHOD(como_crypto_md5) {
	size_t length;
	const char *str = duk_require_lstring(ctx, 0, &length);
	unsigned char digest[16];
	char out[20];
	mbedtls_md5( (unsigned char *) str, length, digest );
	int i;
	for( i = 0; i < 16; i++ ) {
		sprintf(out+i*2, "%02x", digest[i]);
	}

	duk_push_string(ctx, (const char *)out);
	return 1;
}

COMO_METHOD(como_crypto_sha1) {
	size_t length;
	const char *str = duk_require_lstring(ctx, 0, &length);
	unsigned char digest[20];
	char out[20];

	mbedtls_sha1( (unsigned char *) str, length, digest );
	int i;
	for( i = 0; i < 20; i++ ) {
		sprintf(out+i*2, "%02x", digest[i]);
	}
	duk_push_string(ctx, (const char *)out);
	return 1;
}

COMO_METHOD(como_crypto_sha256) {
	size_t length;
	const char *str = duk_require_lstring(ctx, 0, &length);
	unsigned char digest[32];
	char out[32];

	mbedtls_sha256( (unsigned char *) str, length, digest, 0);
	int i;
	for( i = 0; i < 32; i++ ) {
		sprintf(out+i*2, "%02x", digest[i]);
	}
	duk_push_string(ctx, (const char *)out);
	return 1;
}

COMO_METHOD(como_crypto_sha512) {
	size_t length;
	const char *str = duk_require_lstring(ctx, 0, &length);
	unsigned char digest[64];
	char out[64];

	mbedtls_sha512( (unsigned char *) str, length, digest, 0);
	int i;
	for( i = 0; i < 64; i++ ) {
		sprintf(out+i*2, "%02x", digest[i]);
	}
	duk_push_string(ctx, (const char *)out);
	return 1;
}

static const duk_function_list_entry como_crypto_funcs[] = {
    { "md5"            , como_crypto_md5,             1 },
    { "sha1"           , como_crypto_sha1,            1 },
    { "sha256"         , como_crypto_sha256,          1 },
    { "sha512"         , como_crypto_sha512,          1 },
    { NULL, NULL, 0 }
};

static int init_binding_crypto(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_crypto_funcs);
    return 1;
}
