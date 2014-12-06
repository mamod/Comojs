#include <sys/types.h>

#if !defined(MIN)
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

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
    BUFFER,
    RAW
};

/*=============================================================================
  UTF8 slice
 ============================================================================*/
static const int como_utf8_slice(duk_context *ctx, char *buffer,
                                   size_t start, size_t end) {
    
    unsigned slen = end - start;
    if (slen <= 0) {
        duk_push_string(ctx, "");
        return 1;
    }
    
    const char* src = buffer + start;

    int last = 12;
    int lastNB = 0;
    int fixPoint = 0;
    if (last > slen) last = slen;

    while(last){
        int nb;
        nb = UTF8ExtraBytes[(unsigned char)src[slen - last]];
        if (nb == 0 && lastNB == 0){
            last--;
            continue;
        } else {
            if (nb > 0){ fixPoint = last; lastNB = nb;
            } else {
                lastNB--;
                if (lastNB == 0) fixPoint = 0;
            }
        }
        last--;
    }

    if (fixPoint > 0) slen -= fixPoint;
    duk_push_lstring(ctx, src, slen);
    return 1;
}

/*=============================================================================
  HEX slice
 ============================================================================*/
static const int como_hex_slice(duk_context *ctx, char *buffer,
                                   size_t start, size_t end) {
    
    char* src = buffer + start;
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
    return 1;
}

/*=============================================================================
  BASE64 slice
 ============================================================================*/
static const int como_base64_slice(duk_context *ctx, char *buffer,
                                   size_t start, size_t end) {
    
    unsigned slen = end - start;
    if (slen <= 0) {
        duk_push_string(ctx, "");
        return 1;
    }
    
    const char* src = buffer + start;
    duk_push_lstring(ctx, src, slen);
    duk_base64_encode(ctx, -1);
    return 1;
}

/*=============================================================================
  ASCII slice
 ============================================================================*/
void force_ascii_slow(const char *src, char *dst, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        dst[i] = src[i] & 0x7f;
    }
}

static const int como_ascii_slice(duk_context *ctx, char *buffer,
                                   size_t start, size_t end) {

    size_t len = end - start;

    char *src  = buffer + start;
    
    char *dst = (char *)malloc(len);
    if (dst == NULL){
        assert(0 && "ascii slice out of memory");
    }

    //get character length
    size_t charLen   = duk_get_length(ctx, 4);
    duk_pop_n(ctx, 4);

    //only force ascii if byteslength > chars length
    if (len > charLen){
        //dst = (char *)malloc(len);
        force_ascii_slow(src, dst, len);
    }

    duk_push_lstring(ctx, dst, len);
    free(dst);
    return 1;
}

/*=============================================================================
  COMO BUFFER slice
 ============================================================================*/
static const int como_buffer_slice(duk_context *ctx) {
    //buffer
    size_t sz;
    char *buffer = duk_require_buffer(ctx, 0, &sz);

    //encoding //start //end
    enum Encodings encoding = duk_require_int(ctx, 1);
    size_t start            = duk_require_int(ctx, 2);
    size_t end              = duk_require_int(ctx, 3);

    switch (encoding) {
        case UTF8   : return como_utf8_slice(ctx, buffer, start, end);
        case HEX    : return como_hex_slice(ctx, buffer, start, end);
        case BASE64 : return como_base64_slice(ctx, buffer, start, end);
        case ASCII  : return como_ascii_slice(ctx, buffer, start, end);
        case BINARY : return como_ascii_slice(ctx, buffer, start, end);
    }

    duk_error(ctx, DUK_ERR_TYPE_ERROR, "Unknown Encoding");
    return 1;
}

/*=============================================================================
  UTF8 write
 ============================================================================*/
size_t como_utf8_write (size_t max,
                             const char* src,
                             const char* srcEnd,
                             char* dst,
                             char* dstEnd){
    
    size_t length = 0;
    size_t offset = 0;
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
            length -= nb + 1;
            break;
        }
        
        while (nb-- != -1) *dst++ = *src++;
    }
    
    return length;
}

/*=============================================================================
  HEX write
 ============================================================================*/
size_t como_hex_write (size_t max_length,
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
        if (!~a || !~b) return 0;
        dst2[i] = a * 16 + b;
    }
    
    return max;
}

/*=============================================================================
  BASE64 write
 ============================================================================*/
size_t como_base64_write (const char* start,
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

/*=============================================================================
  ASCII write
 ============================================================================*/
size_t como_ascii_write (const char* start,
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

/*=============================================================================
  COMO BUFFER write
 ============================================================================*/
static const int como_buffer_write(duk_context *ctx) {

    //buffer
    size_t bufferSize;
    void *buffer = duk_require_buffer(ctx, 0, &bufferSize);

    //string & string bytes length
    size_t stringSize;
    const char *string  = duk_require_lstring(ctx, 1, &stringSize);

    /* string length
       we need raw char length value to check if the string contains 
       wide characters
    */
    size_t charLen = duk_get_length(ctx, 1);
    int isAscii = (charLen == stringSize);

    //encoding //start //end
    enum Encodings encoding    = duk_require_int(ctx, 2);
    size_t offset              = duk_to_int(ctx, 3);
    size_t length              = duk_to_int(ctx, 4);

    if (length > stringSize){
        length = stringSize;
    }

    size_t max_length = MIN(length, MIN(bufferSize - offset, stringSize));

    if (max_length && offset >= bufferSize) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Offset is out of bounds");
        return 1;
    }
    
    if (encoding == HEX ){
        if ((charLen % 2) != 0) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid Hex string");
        }
    }

    if (isAscii && (encoding == ASCII || 
                    encoding == UTF8 || 
                    encoding == BINARY)) {

        encoding = RAW;
    } else if (isAscii && (encoding == BASE64)){
        //nothing for now
    }

    //printf("START : %u, END : %u\n", offset, stringSize);
    char *start = buffer + offset;
    char *dst = start;
    char *dstEnd = dst + max_length;
    const char *src = string;
    const char *srcEnd = src + stringSize;

    size_t ret;
    switch (encoding) {
        case RAW   : 
            memcpy(dst, src, max_length);
            ret = max_length;
            break;
        case UTF8   : 
            ret = como_utf8_write(max_length, src, srcEnd, dst, dstEnd);
            break;
        case HEX    : 
            ret = como_hex_write(max_length, src, srcEnd, dst, dstEnd);
            break;
        case BASE64 : 
            ret = como_base64_write(start, src, srcEnd, dst, dstEnd);
            break;
        case ASCII  : 
            ret = como_ascii_write(start, src, srcEnd, dst, dstEnd);
            break;
        case BINARY :
            ret = como_ascii_write(start, src, srcEnd, dst, dstEnd);
            break;
        default     : 
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Unknown Encoding");
    }

    duk_push_int(ctx, ret);
    return 1;
}

/*=============================================================================
  BUFFER fill
 ============================================================================*/
static const int como_buffer_fill(duk_context *ctx) {
    
    //buffer
    size_t bufferSize;
    void *buffer = duk_require_buffer(ctx, 0, &bufferSize);

    size_t start, end, at_length, length;
    
    start = duk_require_int(ctx, 2);
    end   = duk_require_int(ctx, 3);

    length = end - start;
    
    //is it a number
    if (duk_is_number(ctx, 1)) {
        int value = duk_get_int(ctx, 1);
        value &= 255;
        memset(buffer + start, value, length);
        return 1;
    }

    //string
    const char *at = duk_require_lstring(ctx, 1, &at_length);

    //optimize single ascii character case
    if (at_length == 1) {
        int value = (int)at[0];
        memset(buffer + start, value, length);
        return 1;
    }
    
    size_t in_there = at_length;
    char* ptr = buffer + start + at_length;
    memcpy(buffer + start, at, MIN(at_length, length));
    if (at_length >= length) {
        return 1;
    }
    
    while (in_there < length - in_there) {
        memcpy(ptr, buffer + start, in_there);
        ptr += in_there;
        in_there *= 2;
    }
    
    if (in_there < length) {
        memcpy(ptr, buffer + start, length - in_there);
        in_there = length;
    }
    
    duk_push_true(ctx);
    return 1;
}

/*=============================================================================
  BUFFER Copy
 ============================================================================*/
static const int como_buffer_copy(duk_context *ctx) {
    
    //buffer
    size_t source_length;
    void *source = duk_require_buffer(ctx, 0, &source_length);

    if (!duk_is_buffer(ctx, 1)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "first arg should be a Buffer");
    }

    size_t target_length;
    void *target_data = duk_require_buffer(ctx, 1, &target_length);

    size_t target_start = duk_get_int(ctx, 2);
    size_t source_start = duk_get_int(ctx, 3);
    size_t source_end   = duk_get_int(ctx, 4);
    
    size_t to_copy = MIN(MIN(source_end - source_start,
        target_length - target_start),
        source_length - source_start);
    
    memcpy(target_data + target_start,
        source + source_start,
        to_copy);
    
    duk_push_number(ctx, to_copy);
    return 1;
}

/*=============================================================================
  COMO BUFFER write
 ============================================================================*/
static const int como_buffer_create(duk_context *ctx) {
    //string
    size_t length;
    const char *string  = duk_require_lstring(ctx, 0, &length);

    //create new buffer
    duk_push_buffer(ctx, length, 1);
    duk_insert(ctx, 0); //insert created buffer at index 0
    
    //start //end
    duk_push_int(ctx, 0);
    duk_push_int(ctx, length);

    //stack => [buffer string encoding start end]
    //dump_stack(ctx, "NEW BUFFER");

    //call buffer_writ c function
    como_buffer_write(ctx);
    //stack => [buffer string encoding start end new_size]
    size_t new_size = duk_require_int(ctx, 5);
    duk_pop_n(ctx, 5); //pop all exept buffer
    
    //printf("%u, %u\n", new_size, length);
    if (new_size < length){
        //new created buffer is smaller 
        //resize this buffer
        duk_resize_buffer(ctx, 0, new_size);
    }

    size_t out_size;
    duk_to_fixed_buffer(ctx, 0, &out_size);
    assert(new_size == out_size);

    return 1;
}

/*=============================================================================
  COMO BUFFER compare
 ============================================================================*/
static const int como_buffer_compare(duk_context *ctx) {
    
    //buffer a
    size_t obj_a_len;
    void *obj_a_data = duk_require_buffer(ctx, 0, &obj_a_len);

    size_t obj_b_len;
    void *obj_b_data = duk_require_buffer(ctx, 1, &obj_b_len);

    size_t cmp_length = MIN(obj_a_len, obj_b_len);
    int32_t val = memcmp(obj_a_data, obj_b_data, cmp_length);
    // Normalize val to be an integer in the range of [1, -1] since
    // implementations of memcmp() can vary by platform.
    if (val == 0) {
        if (obj_a_len > obj_b_len) val = 1;
        else if (obj_a_len < obj_b_len) val = -1;
    } else {
        if (val > 0) val = 1;
        else val = -1;
    }

    duk_push_int(ctx, val);
    return 1;
}

/*=============================================================================
  BUFFER foreach
 ============================================================================*/
static const int como_buffer_foreach(duk_context *ctx) {
    
    //buffer
    size_t length;
    char *buffer = duk_require_buffer(ctx, 0, &length);
    duk_remove(ctx, 0); //stack 0 => [ function(){} ]
    
    size_t i;
    size_t c = 0;
    for (i = 0; i < length; i++){
        char Str[5];
        int nb = UTF8ExtraBytes[ (unsigned char)buffer[i] ];
        Str[0] = (unsigned char)buffer[i];
        if (nb > 0){
            
            //there is no 
            if (i+nb >= length){
                return 1;
            }

            int x = 0;
            while ( x++ != nb ) {
                Str[x] = (unsigned char)buffer[++i];
            }
        }
        
        duk_dup(ctx, 0);
        duk_push_lstring(ctx, Str, nb+1);
        duk_push_uint(ctx, c++);
        duk_call(ctx, 2);
        
        if (duk_check_type(ctx, 1, DUK_TYPE_BOOLEAN)) {
            if (!duk_get_boolean(ctx, 1)) break;
        }

        duk_pop(ctx); //stack => [ function cb (){} ]
        //dump_stack(ctx, "DUP");
    }
    
    return 1;
}

static const duk_function_list_entry como_buffer_funcs[] = {
    { "slice", como_buffer_slice,     4 },
    { "write", como_buffer_write,     5 },
    { "fill", como_buffer_fill,       4 },
    { "copy", como_buffer_copy,       5 },
    { "create", como_buffer_create,   2 },
    { "compare", como_buffer_compare, 2 },
    { "foreach", como_buffer_foreach, 2 },
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
    { "raw"   , RAW    },
    { NULL,     0      }
};

static const int init_binding_buffer(duk_context *ctx) {
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, como_buffer_funcs);
    duk_push_object(ctx);
    duk_put_number_list(ctx, -1, como_buffer_constants);
    duk_put_prop_string(ctx, -2, "encodings");
    return 1;
}
