
static const unsigned int kMaxLength =
    sizeof(duk_int32_t) == sizeof(duk_intptr_t) ? 0x3fffffff : 0x7fffffff;

#if !defined(MIN)
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define CHECK_NOT_OOB(r)                                                    \
  do {                                                                      \
    if (!(r)) duk_error(ctx, DUK_ERR_RANGE_ERROR, "out of range index");    \
  } while (0)


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

enum Encodings {
    ASCII = 1,
    BINARY,
    UTF8,
    BUFFER,
    RAW,
    /* 6 */
    HEX,
    BASE64,
    UCS2
};

inline int ParseArrayIndex(duk_context *ctx, int index, size_t def, size_t *ret) {
    if (duk_is_undefined(ctx, index)){
        *ret = def;
        return 1;
    }

    duk_int32_t tmp_i = duk_to_uint32(ctx, index);

    if (tmp_i < 0) return 0;

    *ret = (size_t)tmp_i;
    return 1;
}

/*=============================================================================
  BUFFER fill
 ============================================================================*/
COMO_METHOD(como_buffer_fill) {

    //buffer
    size_t bufferSize;
    void *buffer = duk_require_buffer_data(ctx, 0, &bufferSize);

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

    if (at_length == 0) return 1;

    //optimize single ascii character case
    if (at_length == 1) {
        int value = (int)at[0];
        memset(buffer + start, value, length);
        return 1;
    }

    size_t in_there = at_length;
    char* ptr = buffer + start + at_length;
    memcpy(buffer + start, at, MIN(at_length, length));
    if (at_length >= length) return 1;

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
COMO_METHOD(como_buffer_copy) {
    size_t source_length;
    void *source = duk_require_buffer_data(ctx, 0, &source_length);

    if (!duk_is_buffer(ctx, 1)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "first arg should be a Buffer");
    }

    size_t target_length;
    void *target_data = duk_require_buffer_data(ctx, 1, &target_length);

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
  COMO BUFFER compare
 ============================================================================*/
COMO_METHOD(como_buffer_compare) {
    //buffer a
    size_t obj_a_len;
    void *obj_a_data = duk_require_buffer_data(ctx, 0, &obj_a_len);

    size_t obj_b_len;
    void *obj_b_data = duk_require_buffer_data(ctx, 1, &obj_b_len);

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
COMO_METHOD(como_buffer_foreach) {
    duk_push_this(ctx);
    size_t length;
    char *buffer = duk_require_buffer_data(ctx, 0, &length);
    duk_remove(ctx, -1); //stack 0 => [ function(){} ]

    size_t i;
    size_t c = 0;
    for (i = 0; i < length; i++){
        char Str[5];
        int nb = UTF8ExtraBytes[ (unsigned char)buffer[i] ];
        Str[0] = (unsigned char)buffer[i];
        if (nb > 0){
            if (i+nb >= length) return 1;
            int x = 0;
            while ( x++ != nb ) Str[x] = (unsigned char)buffer[++i];
        }

        duk_dup(ctx, 0);
        duk_push_lstring(ctx, Str, nb+1);
        duk_push_uint(ctx, c++);
        duk_call(ctx, 2);

        if (duk_check_type(ctx, 1, DUK_TYPE_BOOLEAN)) {
            if (!duk_get_boolean(ctx, 1)) break;
        }

        duk_pop(ctx); //stack => [ function cb (){} ]
    }

    return 1;
}

/*=============================================================================
  Slice/Encode Functions
 ============================================================================*/
#define COMO_SLICE_START                                                    \
  size_t start;                                                             \
  size_t end;                                                               \
  duk_push_this(ctx);                                                       \
  size_t end_max;                                                           \
  char *src = duk_require_buffer_data(ctx, -1, &end_max);                   \
  CHECK_NOT_OOB(ParseArrayIndex(ctx, 0 /*start_arg*/, 0, &start));          \
  CHECK_NOT_OOB(ParseArrayIndex(ctx, 1 /*end_arg*/, end_max, &end));        \
  if (end < start) end = start;                                             \
  CHECK_NOT_OOB(end <= end_max);                                            \
  size_t length = end - start;                                              \
  src += start;                                                             \
  if (length == 0) { duk_push_string(ctx, ""); return 1; }

#define COMO_SLICE_END duk_to_string(ctx, -1);

COMO_METHOD(como_buffer_base64_slice) {
    COMO_SLICE_START
    duk_push_lstring(ctx, src, length);
    duk_base64_encode(ctx, -1);
    return 1;
}

COMO_METHOD(como_buffer_ascii_slice) {
    COMO_SLICE_START
    char *dst = duk_push_fixed_buffer(ctx, length);
    size_t i;
    for (i = 0; i < length; ++i) {
        dst[i] = src[i] & 0x7f;
    }
    COMO_SLICE_END
    return 1;
}

COMO_METHOD(como_buffer_hex_slice) {
    COMO_SLICE_START
    uint32_t dstlen = length * 2;
    char *dst = duk_push_fixed_buffer(ctx, dstlen);

    uint32_t i;
    uint32_t k;
    for (i = 0, k = 0; k < dstlen; i += 1, k += 2) {
        static const char hex[] = "0123456789abcdef";
        uint8_t val = (uint8_t)(src[i]);
        dst[k + 0] = hex[val >> 4];
        dst[k + 1] = hex[val & 15];
    }

    COMO_SLICE_END
    return 1;
}

COMO_METHOD(como_buffer_utf8_slice) {
    COMO_SLICE_START

    unsigned slen = length;
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
  Write/decode functions
 ============================================================================*/
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

size_t base64_decoded_size(const char *src, size_t size) {
    if (size == 0) return 0;
    if (src[size - 1] == '=') size--;
    if (size > 0 && src[size - 1] == '=') size--;
    return base64_decoded_size_fast(size);
}

size_t base64_decode_slow(char *dst, size_t dstlen,
                          const char *src, size_t srclen) {
  uint8_t hi;
  uint8_t lo;
  size_t i = 0;
  size_t k = 0;
  for (;;) {
#define V(expr)                                                               \
    while (i < srclen) {                                                      \
      const uint8_t c = src[i];                                               \
      lo = unbase64(c);                                                       \
      i += 1;                                                                 \
      if (lo < 64)                                                            \
        break;  /* Legal character. */                                        \
      if (c == '=')                                                           \
        return k;                                                             \
    }                                                                         \
    expr;                                                                     \
    if (i >= srclen)                                                          \
      return k;                                                               \
    if (k >= dstlen)                                                          \
      return k;                                                               \
    hi = lo;
    V(/* Nothing. */);
    V(dst[k++] = ((hi & 0x3F) << 2) | ((lo & 0x30) >> 4));
    V(dst[k++] = ((hi & 0x0F) << 4) | ((lo & 0x3C) >> 2));
    V(dst[k++] = ((hi & 0x03) << 6) | ((lo & 0x3F) >> 0));
#undef V
  }
  assert(0 && "unreachable");
}

size_t base64_decode_fast(char *dst, size_t dstlen,
                          const char *src, size_t srclen,
                          const size_t decoded_size) {
  const size_t available = dstlen < decoded_size ? dstlen : decoded_size;
  const size_t max_i = srclen / 4 * 4;
  const size_t max_k = available / 3 * 3;
  size_t i = 0;
  size_t k = 0;
  while (i < max_i && k < max_k) {
    const uint32_t v =
        unbase64(src[i + 0]) << 24 |
        unbase64(src[i + 1]) << 16 |
        unbase64(src[i + 2]) << 8 |
        unbase64(src[i + 3]);
    // If MSB is set, input contains whitespace or is not valid base64.
    if (v & 0x80808080) {
      break;
    }
    dst[k + 0] = ((v >> 22) & 0xFC) | ((v >> 20) & 0x03);
    dst[k + 1] = ((v >> 12) & 0xF0) | ((v >> 10) & 0x0F);
    dst[k + 2] = ((v >>  2) & 0xC0) | ((v >>  0) & 0x3F);
    i += 4;
    k += 3;
  }
  if (i < srclen && k < dstlen) {
    return k + base64_decode_slow(dst + k, dstlen - k, src + i, srclen - i);
  }
  return k;
}


size_t como_base64_decode(char *dst, size_t dstlen,
                     const char *src, size_t srclen) {
  const size_t decoded_size = base64_decoded_size(src, srclen);
  return base64_decode_fast(dst, dstlen, src, srclen, decoded_size);
}


size_t como_ascii_decode (char *dest, size_t len, const char *src){
    int i = 0;
    for (i = 0; i < len; i++) {
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
        *dest++ = ch;
    }

    return len;
}

size_t como_hex_decode(char *buf, size_t len, const char *src, const size_t srcLen) {
    size_t i;
    for (i = 0; i < len && i * 2 + 1 < srcLen; ++i) {
        unsigned a = hex2bin(src[i * 2 + 0]);
        unsigned b = hex2bin(src[i * 2 + 1]);
        if (!~a || !~b) return i;
        buf[i] = a * 16 + b;
    }
    return i;
}

size_t como__StringBytesWrite(duk_context *ctx, char *buf, size_t buflen,
                          int val, enum Encodings encoding, int *chars_written) {

    size_t nbytes;
    const char *data = duk_require_lstring(ctx, val, &nbytes);
    size_t strLength = duk_get_length(ctx, val);

    const int isOneByte = (strLength == nbytes);
    const size_t external_nbytes = nbytes;

    if (nbytes > buflen) nbytes = buflen;

    switch (encoding) {
        case UTF8: memcpy(buf, data, nbytes); break;
        case ASCII:
        case BINARY:
        case BUFFER: {
            if (isOneByte) {
                memcpy(buf, data, nbytes);
            } else {
                nbytes = como_ascii_decode(buf, strLength, data);
            }
            break;
        }

        case BASE64: {
            nbytes = como_base64_decode(buf, buflen, data, external_nbytes);
            break;
        }

        case HEX: {
            nbytes = como_hex_decode(buf, buflen, data, strLength);
            break;
        }

        default: assert(0 && "unknown encoding");
    }

    return nbytes;
}

int como_StringWrite(duk_context *ctx, enum Encodings encoding) {

    duk_push_this(ctx);

    size_t ts_obj_length;
    void *ts_obj_data = duk_require_buffer_data(ctx, -1, &ts_obj_length);

    if (!duk_is_string(ctx, 0)){
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Argument must be a string");
    }

    size_t strLength = duk_get_length(ctx, 0);

    if (encoding == HEX && strLength % 2 != 0) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid hex string");
    }

    size_t offset;
    size_t max_length;

    CHECK_NOT_OOB(ParseArrayIndex(ctx, 1, 0, &offset));
    CHECK_NOT_OOB(ParseArrayIndex(ctx, 2, ts_obj_length - offset, &max_length));

    max_length = MIN(ts_obj_length - offset, max_length);

    if (max_length == 0) {
        duk_push_int(ctx, 0);
        return 1;
    }

    if (offset >= ts_obj_length) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Offset is out of bounds");
    }

    uint32_t written = 0;
    written = como__StringBytesWrite(ctx, ts_obj_data + offset,
                                 max_length, 0 /* string index */, encoding, NULL);

    duk_push_uint(ctx, written);
    return 1;
}

COMO_METHOD(como_buffer_base64_write) { return como_StringWrite(ctx, BASE64); }
COMO_METHOD(como_buffer_utf8_write)   { return como_StringWrite(ctx, UTF8);   }
COMO_METHOD(como_buffer_ascii_write)  { return como_StringWrite(ctx, ASCII);  }
COMO_METHOD(como_binary_write)        { return como_StringWrite(ctx, ASCII);  }
COMO_METHOD(como_buffer_hex_write)    { return como_StringWrite(ctx, HEX);    }

COMO_METHOD(como_buffer_new_from_string) {
    size_t nbytes;
    const char *string = duk_require_lstring(ctx, 0, &nbytes);
    size_t strLength = duk_get_length(ctx, 0);

    enum Encodings enc = duk_require_int(ctx, 1);

    // StringBytes::Size;
    size_t length = 0;
    switch(enc) {
        case UTF8   : length = nbytes;        break;
        case ASCII  : //fallthrow
        case BUFFER : //fallthrow
        case BINARY : length = strLength;     break;
        case HEX    : length = strLength / 2; break;
        case BASE64 : length = base64_decoded_size(string, strLength); break;
        default : assert(0 && "unknown encoding");
    }

    size_t actual = 0;
    size_t out_len = 0;

    char *data = NULL;

    if (length > 0) {
        data = duk_push_dynamic_buffer(ctx, length);
        if (data == NULL) {
            duk_push_fixed_buffer(ctx, 0);
            duk_push_buffer_object(ctx, -1, 0, 0, DUK_BUFOBJ_NODEJS_BUFFER);
            return 1;
        }

        actual = como__StringBytesWrite(ctx, data, length, 0, enc, NULL);
        assert(actual <= length);

        if (actual < length) {
            duk_resize_buffer(ctx, -1, actual);
        }

        duk_to_fixed_buffer(ctx, -1, &out_len);
    } else {
        duk_push_fixed_buffer(ctx, 0);
    }

    duk_push_buffer_object(ctx, -1, 0, out_len, DUK_BUFOBJ_NODEJS_BUFFER);
    return 1;
}

COMO_METHOD(como_bytelength_utf8) {
    duk_size_t len;
    duk_to_buffer(ctx, 0, &len);
    duk_push_uint(ctx, len);
    return 1;
}

static const duk_function_list_entry como_buffer_funcs[] = {
    {"fill", como_buffer_fill,                         4},
    {"copy", como_buffer_copy,                         5},
    {"compare", como_buffer_compare,                   2},
    {"foreach", como_buffer_foreach,                   2},

    {"byteLengthUtf8", como_bytelength_utf8,           1},

    {"hexSlice", como_buffer_hex_slice,                2},
    {"base64Slice", como_buffer_base64_slice,          2},
    {"asciiSlice", como_buffer_ascii_slice,            2},
    {"utf8Slice", como_buffer_utf8_slice,              2},

    {"utf8Write", como_buffer_utf8_write,              3},
    {"hexWrite", como_buffer_hex_write,                3},
    {"base64Write", como_buffer_base64_write,          3},
    {"asciiWrite", como_buffer_ascii_write,            3},
    {"binaryWrite", como_binary_write,                 3},
    {"createFromString", como_buffer_new_from_string,  2},

    {NULL, NULL,                                       0}
};

static const duk_number_list_entry como_buffer_constants[] = {
    {"hex", HEX             },
    {"utf8", UTF8           },
    {"utf-8", UTF8          },
    {"ucs2" , UCS2          },
    {"ascii", ASCII         },
    {"base64", BASE64       },
    {"binary", BINARY       },
    {"buffer", BUFFER       },
    {"raw", RAW             },
    {NULL, 0                }
};

static int init_binding_buffer(duk_context *ctx) {
    duk_push_object(ctx);

    duk_push_number(ctx, kMaxLength);
    duk_put_prop_string(ctx, -2, "kMaxLength");

    duk_push_number(ctx, kMaxLength / 4);
    duk_put_prop_string(ctx, -2, "kStringMaxLength");

    duk_put_function_list(ctx, -1, como_buffer_funcs);
    duk_push_object(ctx);
    duk_put_number_list(ctx, -1, como_buffer_constants);
    duk_put_prop_string(ctx, -2, "encodings");

    return 1;
}
