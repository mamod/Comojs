#include <sys/types.h>

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define _GET_NUM duk_get_int

static const unsigned long ReplacementChar = 0x0000FFFDUL;
static const unsigned long MaximumChar  = 0x7FFFFFFFUL;

static const unsigned long offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char UTF8ExtraBytes[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/* supports regular and URL-safe base64 */
static const int unbase64_table[] =
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-1,-1,-2,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,62,-1,63
,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1
,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14
,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,63
,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40
,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};
#define unbase64(x) unbase64_table[(uint8_t)(x)]


unsigned hex2bin(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

const enum Encodings {
    ASCII = 1,
    UTF8,
    HEX,
    BASE64,
    BINARY,
    UCS2,
    BUFFER
};

typedef struct {
    char*  base;
    size_t  len;
} Buffer;

static const int _buffer_SlowBuffer(duk_context *ctx) {
    size_t length = (size_t)duk_get_int(ctx, 0);
    Buffer *buffer = malloc(sizeof(*buffer));
    
    char *data_ = (char *)malloc(length);
    if (data_ == NULL){
        COMO_FATAL_ERROR("Out of memory");
    }
    memset(data_, 0, length);
    buffer->base = data_;
    buffer->len = length;
    duk_push_pointer(ctx, buffer);
    return 1;
}

static const int _buffer_read(duk_context *ctx) {
    Buffer *buf = duk_get_pointer(ctx, 0);
    duk_push_string(ctx, buf->base);
    return 1;
}

size_t _buffer_base64_write (const char* start,
                             const char* src,
                             const char* srcEnd,
                             char* dst,
                             char* dstEnd){
    
    char a, b, c, d;
    while (src < srcEnd && dst < dstEnd) {
        int remaining = srcEnd - src;
        while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
        if (remaining == 0 || *src == '=') break;
        a = unbase64(*src++);
        while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
        if (remaining <= 1 || *src == '=') break;
        b = unbase64(*src++);
        *dst++ = (a << 2) | ((b & 0x30) >> 4);
        if (dst == dstEnd) break;
        while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
        if (remaining <= 2 || *src == '=') break;
        c = unbase64(*src++);
        *dst++ = ((b & 0x0F) << 4) | ((c & 0x3C) >> 2);
        if (dst == dstEnd) break;
        while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
        if (remaining <= 3 || *src == '=') break;
        d = unbase64(*src++);
        *dst++ = ((c & 0x03) << 6) | (d & 0x3F);
    }
    
    return dst - start;
}

size_t _buffer_utf8_write (size_t max,
                             const char* src,
                             const char* srcEnd,
                             char* dst,
                             char* dstEnd){
    
    int length = 0;
    int offset = 0;
    while (src < srcEnd && dst < dstEnd) {
        unsigned long ch;
        int nb;
        nb = UTF8ExtraBytes[(unsigned char)*src];
        /* fast case */
        if (nb == 0){
            length++;
            *dst++ = *src++;
            continue;
        }
        
        length += nb + 1;
        if (length > max ){
            length-= nb + 1;
            break;
        }
        
        while (nb-- != -1){
            *dst++ = *src++;
        }
    }
    
    return length;
}

size_t _buffer_ascii_write (const char* start,
                             const char* src,
                             const char* srcEnd,
                             char* dst,
                             char* dstEnd){
    
    while (src < srcEnd && dst < dstEnd) {
        unsigned long ch;
        int nb;
        nb = UTF8ExtraBytes[(unsigned char)*src];
        ch = 0;
        /* fast case */
        if (nb == 0){
            ch += (unsigned char)*src++;
        } else {
            switch (nb) {
                /* these fall through deliberately */
                case 5: ch += (unsigned char)*src++; ch <<= 6;
                case 4: ch += (unsigned char)*src++; ch <<= 6;
                case 3: ch += (unsigned char)*src++; ch <<= 6;
                case 2: ch += (unsigned char)*src++; ch <<= 6;
                case 1: ch += (unsigned char)*src++; ch <<= 6;
                case 0: ch += (unsigned char)*src++;
            }
        }
        
        ch -= offsetsFromUTF8[nb];
        *dst++ = ch;
    }
    
    return dst - start;
}

size_t _buffer_hex_write (size_t max_length,
                             const char* src,
                             const char* srcEnd,
                             char* dst,
                             char* dstEnd){
    
    char* dst2 = dst;
    const char* src2 = src;
    size_t max = strlen(src) / 2;
    
    if (max > max_length){
        max = max_length;
    }
    
    size_t i;
    for (i = 0; i < max; ++i) {
        unsigned a = hex2bin(src2[i * 2 + 0]);
        unsigned b = hex2bin(src2[i * 2 + 1]);
        if (!~a || !~b) {
            return 0;
        }
        
        dst2[i] = a * 16 + b;
    }
    
    return max;
}

static const int _buffer_write(duk_context *ctx) {
    Buffer *buffer = duk_get_pointer(ctx, 0);
    enum Encodings type = duk_get_int(ctx, 1);
    size_t length;
    const char *string = duk_get_lstring(ctx, 2, &length);
    size_t offset = duk_get_int(ctx, 3);
    size_t max = duk_get_int(ctx, 4);
    
    /*
     * this gives the character size of string not bytes length
     * we will check if there is wide characters by comparing the
     * bytes length with chars length
     */
    size_t charLen = duk_get_length(ctx, 2);
    int is_extern = length == charLen;
    
    size_t max_length = max;
    max_length = MIN(length, MIN(buffer->len - offset, max_length));
    
    if (max_length && offset >= buffer->len) {
        COMO_FATAL_ERROR("Offset is out of bounds");
    }
    
    if (type == HEX ){
        if ((length % 2) != 0) {
            assert( 0 && "Invalid Hex string");
        }
    }
    
    char* start = buffer->base + offset;
    char* dst = start;
    char* dstEnd = dst + max_length;
    const char* src = string;
    const char* srcEnd = src + length;
    
    size_t ret;
    switch(type) {
        case UTF8:
            ret = _buffer_utf8_write(max_length, src, srcEnd, dst, dstEnd);
            break;
        case HEX:
            ret = _buffer_hex_write(max_length, src, srcEnd, dst, dstEnd);
            break;
        case ASCII:
            ret = _buffer_ascii_write(start, src, srcEnd, dst, dstEnd);
            break;
        case BASE64:
            ret = _buffer_base64_write(start, src, srcEnd, dst, dstEnd);
            break;
        case BINARY:
            COMO_FATAL_ERROR("BINAR");
            break;
         case UCS2:
            COMO_FATAL_ERROR("UCS2");
            break;
        default :
            COMO_FATAL_ERROR("unknown encoding");
    }
    
    duk_push_number(ctx, ret);
    return 1;
}

static const int _buffer_utf8_slice(duk_context *ctx, Buffer *buffer,
                                   size_t start, size_t end) {
    
    unsigned slen = end - start;
    if (slen <= 0) {
        duk_push_string(ctx, "");
        return 1;
    }
    
    const char* src = buffer->base + start;
    duk_push_lstring(ctx, src, slen);
    return 1;
}

static const int _buffer_hex_slice(duk_context *ctx, Buffer *buffer,
                                   size_t start, size_t end) {
    
    char* src = buffer->base + start;
    uint32_t dstlen = (end - start) * 2;
    if (dstlen == 0) {
        duk_push_string(ctx, "");
        return 1;
    }
    
    char *dst = (char *)malloc(dstlen);
    if (dst == NULL){
        assert(0 && "hexSlice out of memory");
    }
    
    uint32_t i;
    uint32_t k;
    for (i = 0, k = 0; k < dstlen; i += 1, k += 2) {
        static const char hex[] = "0123456789abcdef";
        uint8_t val = (uint8_t)(src[i]);
        dst[k + 0] = hex[val >> 4];
        dst[k + 1] = hex[val & 15];
    }
    
    duk_push_lstring(ctx, dst, dstlen);
    free(dst);
    dst = NULL;
    return 1;
}

static const int _buffer_slice(duk_context *ctx) {
    Buffer *buffer = duk_get_pointer(ctx, 0);
    enum Encodings type = duk_get_int(ctx, 1);
    size_t start = duk_get_int(ctx, 2);
    size_t end   = duk_get_int(ctx, 3);
    if (buffer == NULL){
        return 1;
    }
    switch(type) {
        case UTF8:
            return _buffer_utf8_slice(ctx, buffer, start, end);
            break;
        case HEX:
            return _buffer_hex_slice(ctx, buffer, start, end);
            break;
        case ASCII:
            
            break;
        case BASE64:
            
            break;
        default :
            COMO_FATAL_ERROR("unknown encoding");
    }
}

static const int _buffer_set(duk_context *ctx) {
    Buffer *buf = duk_get_pointer(ctx, 0);
    int offset = duk_get_int(ctx, 1);
    int ch = (char)_GET_NUM(ctx, 2);
    buf->base[offset] = ch;
    duk_push_true(ctx);
    return 1;
}

static const int _buffer_get(duk_context *ctx) {
    Buffer *buf = duk_get_pointer(ctx, 0);
    int offset = duk_get_int(ctx, 1);
    int ch = buf->base[offset];
    if (ch < 0){
        ch &= 255;
    }
    duk_push_int(ctx, ch);
    return 1;
}

static inline size_t base64_decoded_size_fast(size_t size) {
    size_t remainder = size % 4;
    size = (size / 4) * 3;
    if (remainder) {
        if (size == 0 && remainder == 1) {
            // special case: 1-byte input cannot be decoded
            size = 0;
        } else {
            // non-padded input, add 1 or 2 extra bytes
            size += 1 + (remainder == 3);
        }
    }
    return size;
}

size_t base64_decoded_size(const char* src, size_t size) {
    if (size == 0) return 0;   
    if (src[size - 1] == '=') size--;
    if (size > 0 && src[size - 1] == '=') size--;
    return base64_decoded_size_fast(size);
}

static const int _buffer_byteLength(duk_context *ctx) {
    size_t length;
    const char *string = duk_get_lstring(ctx, 0, &length);
    size_t charLen = duk_get_length(ctx, 0);
    int is_extern = length == charLen;
    
    size_t data_size = 0;
    if (is_extern){
        data_size = length;
        goto out;
    }
    
    enum Encodings encoding = duk_get_int(ctx, 1);
    
    switch (encoding) {
        case BINARY:
        case BUFFER:
        case ASCII:
            data_size = charLen;
            break;
        case UTF8:
            data_size = length;
            break;
        case UCS2:
            data_size = charLen * sizeof(uint16_t);
            break;
        case BASE64: {
            data_size = base64_decoded_size(string, length);
            break;
        }
        case HEX:
            data_size = charLen / 2;
            break;
        default:
            COMO_FATAL_ERROR("unknown encoding");
            break;
    }
    
    out :
    duk_push_number(ctx, data_size);
    return 1;
}


static const int _buffer_fill(duk_context *ctx) {
    Buffer *buf = duk_get_pointer(ctx, 0);
    size_t start, end, at_length, length;
    const char *at = duk_get_lstring(ctx, 1, &at_length);
    start = _GET_NUM(ctx, 2);
    end = _GET_NUM(ctx, 3);
    char *obj_data = buf->base;
    length = end - start;
    
    if (duk_is_number(ctx, 1)) {
        int value = _GET_NUM(ctx, 1);
        value &= 255;
        memset(obj_data + start, value, length);
        return 1;
    }
    
    //optimize single ascii character case
    if (at_length == 1) {
        int value = (int)at[0];
        memset(obj_data + start, value, length);
        return 1;
    }
    
    size_t in_there = at_length;
    char* ptr = obj_data + start + at_length;
    memcpy(obj_data + start, at, MIN(at_length, length));
    if (at_length >= length) {
        return 1;
    }
    
    while (in_there < length - in_there) {
        memcpy(ptr, obj_data + start, in_there);
        ptr += in_there;
        in_there *= 2;
    }
    
    if (in_there < length) {
        memcpy(ptr, obj_data + start, length - in_there);
        in_there = length;
    }
    
    duk_push_true(ctx);
    return 1;
}

static const int _buffer_copy(duk_context *ctx) {
    
    Buffer *source = duk_get_pointer(ctx, 0);
    Buffer *target = duk_get_pointer(ctx, 1);
    size_t target_start = _GET_NUM(ctx, 2);
    size_t source_start = _GET_NUM(ctx, 3);
    size_t source_end = _GET_NUM(ctx, 4);
    
    char* target_data = target->base;
    size_t target_length = target->len;
    
    if (source_start >= source->len) {
        COMO_FATAL_ERROR("sourceStart out of bounds");
    }
    
    if (source_end > source->len) {
        COMO_FATAL_ERROR("sourceEnd out of bounds");
    }
    
    if (source_start > source_end) {
        COMO_FATAL_ERROR("out of range index");
    }
    
    size_t to_copy = MIN(MIN(source_end - source_start,
        target_length - target_start),
        source->len - source_start);
    
    // need to use slightly slower memmove
    //is the ranges might overlap??
    memcpy(target_data + target_start,
        source->base + source_start,
        to_copy);
    
    duk_push_number(ctx, to_copy);
    return 1;
}

static const int _buffer_destroy(duk_context *ctx) {
    Buffer *buf = duk_get_pointer(ctx, 0);
    if (buf != NULL){
        free(buf->base);
        buf->base = NULL;
        buf->len = 0;
        free(buf);
        buf = NULL;
    }
    duk_push_true(ctx);
    return 1;
}

static const duk_function_list_entry como_buffer_funcs[] = {
    { "byteLength",  _buffer_byteLength, 2 },
    { "SlowBuffer", _buffer_SlowBuffer, 1 },
    { "write", _buffer_write, 5 },
    { "slice", _buffer_slice, 4 },
    { "read", _buffer_read, 1 },
    { "set", _buffer_set, 3},
    { "get", _buffer_get, 2},
    { "fill", _buffer_fill, 4},
    { "copy", _buffer_copy, 5},
    { "destroy", _buffer_destroy, 1 },
    { NULL, NULL, 0 }
};

static const duk_number_list_entry como_buffer_constants[] = {
    { "hex",    HEX    },
    { "utf8",   UTF8   },
    { "utf-8",  UTF8   },
    { "ucs2" ,  UCS2   },
    { "ascii",  ASCII  },
    { "base64", BASE64 },
    { "binary", BINARY },
    { "buffer", BUFFER },
    { NULL, 0 }
};

static const int init_binding_buffer(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_buffer_funcs);
    duk_put_number_list(ctx, -1, como_buffer_constants);
    return 1;
}
