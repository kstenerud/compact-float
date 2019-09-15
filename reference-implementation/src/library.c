#include "compact_float/compact_float.h"
#include <math.h>
#include <vlq/vlq.h>

#define KSLog_FileDesriptor STDOUT_FILENO
#define KSLog_LocalMinLevel KSLOG_LEVEL_TRACE
#include <kslog/kslog.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

// Normal
// s 00eeeeeeee (0)m x 53
// s 01eeeeeeee (0)m x 53
// s 10eeeeeeee (0)m x 53
// s 1100eeeeeeee (100)m x 51
// s 1101eeeeeeee (100)m x 51
// s 1110eeeeeeee (100)m x 51

// Inf
// s 11110 xxx...

// NaN
// s 111110 xxx...

// sNaN
// s 111111 xxx...

// Technically, nan payload is ignored.

#define SIZE_SIGNIFICAND_NORMAL 53
#define MASK_SIGNIFICAND_NORMAL ((1ULL << SIZE_SIGNIFICAND_NORMAL) - 1)
#define SIZE_SIGNIFICAND_EXTENDED 51
#define MASK_SIGNIFICAND_EXTENDED ((1ULL << SIZE_SIGNIFICAND_EXTENDED) - 1)
#define PREFIX_SIGNIFICAND_EXTENDED (1UL << SIZE_SIGNIFICAND_NORMAL)
#define SIZE_EXPONENT 10
#define MASK_EXPONENT ((1ULL << SIZE_EXPONENT) - 1)
#define SHIFT_COMBINATION 58
#define SIZE_COMBINATION 5
#define MASK_COMBINATION ((1 << SIZE_COMBINATION) - 1)
#define VALUE_COMBINATION_INF 0x1e
#define VALUE_COMBINATION_NAN 0x1f
#define SHIFT_SIGNALING_NAN 57
#define MASK_SIGNALING_NAN (1UL << SHIFT_SIGNALING_NAN)

#define SHIFT_EXTENDED 61
#define MASK_EXTENDED (3UL << SHIFT_EXTENDED)
#define VALUE_EXTENDED (3UL << SHIFT_EXTENDED)

#define EXPONENT_BIAS 398
#define EXPONENT_BIASED_MAX 0x2ff
#define EXPONENT_UNBIASED_MAX 384
#define SIGNIFICAND_MAX 9999999999999999UL


#define shift_right_max(X) ((X) >> (sizeof(X) * 8 - 1))

static inline int get_sign(uint64_t value)
{
    return (int)shift_right_max(value) & 1;
}

static inline bool is_decimal_infinity(uint64_t value)
{
    return ((value >> SHIFT_COMBINATION) & MASK_COMBINATION) == VALUE_COMBINATION_INF;
}

static inline bool is_decimal_nan(uint64_t value)
{
    return ((value >> SHIFT_COMBINATION) & MASK_COMBINATION) == VALUE_COMBINATION_NAN;
}

static inline uint64_t absolute_value(int64_t value)
{
    const int64_t mask = shift_right_max(value);
    return (value + mask) ^ mask;
}

static int encode_nan(unsigned is_signaling, uint8_t* dst, int dst_length)
{
    const int encoded_length = 2;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    dst[0] = 0x80;
    dst[1] = is_signaling;
    return encoded_length;
}

static int encode_infinity(int sign, uint8_t* dst, int dst_length)
{
    const int encoded_length = 2;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    dst[0] = 0x80;
    dst[1] = 0x02 | (shift_right_max(sign) & 1);
    return encoded_length;
}

static int encode_zero(int sign, uint8_t* dst, int dst_length)
{
    const int encoded_length = 1;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    dst[0] = 0x02 | (shift_right_max(sign) & 1);
    return encoded_length;
}

static int encode_number(unsigned significand_sign, uint64_t significand, uint64_t exponent_sign, uint64_t exponent, uint8_t* dst, int dst_length)
{
    uint64_t exponent_field = significand_sign | (exponent_sign << 1) | (absolute_value(exponent) << 2);

    int offset = rvlq_encode_64(exponent_field, dst, dst_length);
    if(offset < 1)
    {
        return offset;
    }

    int bytes_encoded = rvlq_encode_64(significand, dst+offset, dst_length-offset);
    if(bytes_encoded < 1)
    {
        return bytes_encoded;
    }
    offset += bytes_encoded;

    return offset;
}

// ----------
// Public API
// ----------

const char* cfloat_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

int cfloat_binary_encoded_size(double value)
{
    // ANSI_EXTENSION _Decimal64 dvalue = value;
    (void)value;
    return 0;
    // uint64_t significand_vlq = 0;
    // const uint64_t data = get_float_data(value, &significand_vlq);
    // const uint64_t exponent_vlq = data & ~FLOAT_DATA_FLAGS_MASK;
    // const int base_length = (data & EXPONENT_IS_EXTENDED) ? 1 : 0;
    // const int significand_length = (data & SIGNIFICAND_IS_PRESENT) ? lvlq_encoded_size_64(significand_vlq) : 0;

    // return base_length + rvlq_encoded_size_64(exponent_vlq) + significand_length;
}

int cfloat_encode(int64_t exponent, int64_t significand, uint8_t* dst, int dst_length)
{
    return encode_number(get_sign(significand), absolute_value(significand), get_sign(exponent), absolute_value(exponent), dst, dst_length);
}

ANSI_EXTENSION int cfloat_decimal_encode(_Decimal64 dvalue, uint8_t* dst, int dst_length)
{
    if(dvalue == 0.0dd)
    {
        return encode_zero(0, dst, dst_length);
    }
    if(dvalue == -0.0dd)
    {
        return encode_zero(1, dst, dst_length);
    }

    uint64_t uvalue = 0;
    memcpy(&uvalue, &dvalue, sizeof(uvalue));

    if(is_decimal_infinity(uvalue))
    {
        return encode_infinity(get_sign(uvalue), dst, dst_length);
    }

    if(is_decimal_nan(uvalue))
    {
        unsigned is_signaling = (uvalue >> SHIFT_SIGNALING_NAN) & 1;
        return encode_nan(is_signaling, dst, dst_length);
    }

    uint64_t significand = 0;
    int exponent = 0;
    if((uvalue & MASK_EXTENDED) == VALUE_EXTENDED)
    {
        exponent = (uvalue >> SIZE_SIGNIFICAND_EXTENDED) & MASK_EXPONENT;
        significand = (uvalue & MASK_SIGNIFICAND_EXTENDED) | PREFIX_SIGNIFICAND_EXTENDED;
    }
    else
    {
        exponent = (uvalue >> SIZE_SIGNIFICAND_NORMAL) & MASK_EXPONENT;
        significand = uvalue & MASK_SIGNIFICAND_NORMAL;
    }
    exponent -= EXPONENT_BIAS;

    return encode_number(get_sign(uvalue), significand, get_sign(exponent), absolute_value(exponent), dst, dst_length);
}

ANSI_EXTENSION int cfloat_decimal_decode(const uint8_t* src, int src_length, _Decimal64* value)
{
    if(src_length < 1)
    {
        return 0;
    }

    uint64_t exponent_field = 0;
    int offset = rvlq_decode_64(&exponent_field, src, src_length);
    if(offset < 1)
    {
        return offset;
    }

    if(src[0] == 0x80) // Extended RVLQ
    {
        switch(exponent_field)
        {
            case 0:
                *value = NAN;
                return offset;
            case 1:
                // TODO: Signaling NaN here
                *value = NAN;
                return offset;
            case 2:
                *value = INFINITY;
                return offset;
            case 3:
                *value = -INFINITY;
                return offset;
        }
    }

    uint64_t significand = 0;
    int bytesDecoded = rvlq_decode_64(&significand, src+offset, src_length-offset);
    if(bytesDecoded < 1)
    {
        return bytesDecoded;
    }
    offset += bytesDecoded;

    uint64_t significand_sign = (exponent_field & 1) << 63;
    unsigned exponent_sign = (exponent_field >> 1) & 1;
    uint64_t exponent_abs = exponent_field >> 2;

    if(significand > SIGNIFICAND_MAX)
    {
        return 0;
    }

    if(exponent_abs > EXPONENT_UNBIASED_MAX)
    {
        return 0;
    }

    int exponent_biased = exponent_abs;
    if(exponent_sign)
    {
        exponent_biased = -exponent_biased;
    }
    exponent_biased += EXPONENT_BIAS;
    if(exponent_biased < 0)
    {
        return 0;
    }
    if(exponent_biased > EXPONENT_BIASED_MAX)
    {
        return 0;
    }

    uint64_t uvalue = 0;
    if(significand & PREFIX_SIGNIFICAND_EXTENDED)
    {
        uvalue = significand_sign |
            VALUE_EXTENDED |
            exponent_biased | SIZE_SIGNIFICAND_EXTENDED | 
            (significand & ~PREFIX_SIGNIFICAND_EXTENDED);
    }
    else
    {
        uvalue = significand_sign |
            exponent_biased | SIZE_SIGNIFICAND_NORMAL | 
            significand;
    }

    memcpy(value, &uvalue, sizeof(*value));
    return offset;
}

int cfloat_binary_encode(double fvalue, uint8_t* dst, int dst_length)
{
    if(dst_length < 1)
    {
        return 0;
    }

    return cfloat_decimal_encode(fvalue, dst, dst_length);
}

int cfloat_binary_decode(const uint8_t* src, int src_length, double* value)
{
    ANSI_EXTENSION _Decimal64 dvalue = 0;
    int result = cfloat_decimal_decode(src, src_length, &dvalue);
    if(result >= 1)
    {
        *value = dvalue;
    }
    return result;
}
