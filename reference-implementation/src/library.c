#include "compact_float/compact_float.h"
#include <math.h>
#include <vlq/vlq.h>

// #define KSLog_FileDesriptor STDOUT_FILENO
// #define KSLog_LocalMinLevel KSLOG_LEVEL_TRACE
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
// max sig 2386f26fc0ffff 13*4 += 54

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

#define ENCODED_VALUE_POS_0         0x02
#define ENCODED_VALUE_NEG_0         0x03
#define ENCODED_VALUE_POS_INFINITY  0x02
#define ENCODED_VALUE_NEG_INFINITY  0x03
#define ENCODED_VALUE_QUIET_NAN     0x00
#define ENCODED_VALUE_SIGNALING_NAN 0x01

#define MAX_DIGITS_64_BIT 16


#define shift_right_max(X) ((X) >> (sizeof(X) * 8 - 1))

static inline unsigned get_sign(uint64_t value)
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

static inline int encode_nan(bool is_signaling, uint8_t* dst, int dst_length)
{
    const int encoded_length = 2;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    vlq_extend(dst, 1);
    dst[1] = is_signaling ? ENCODED_VALUE_SIGNALING_NAN : ENCODED_VALUE_QUIET_NAN;
    KSLOG_TRACE("Encoded NaN with signaling %d into %02x %02x", is_signaling, dst[0], dst[1]);
    return encoded_length;
}

static inline int encode_infinity(unsigned sign, uint8_t* dst, int dst_length)
{
    const int encoded_length = 2;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    vlq_extend(dst, 1);
    dst[1] = ENCODED_VALUE_POS_INFINITY | sign;
    KSLOG_TRACE("Encoded infinity with sign %d into %02x %02x", sign, dst[0], dst[1]);
    return encoded_length;
}

static inline int encode_zero(unsigned sign, uint8_t* dst, int dst_length)
{
    const int encoded_length = 1;
    if(dst_length < encoded_length)
    {
        return 0;
    }
    dst[0] = ENCODED_VALUE_POS_0 | sign;
    KSLOG_TRACE("Encoded zero with sign %d into %02x", sign, dst[0]);
    return encoded_length;
}

static int encode_number(unsigned significand_sign, uint64_t significand, uint64_t exponent_sign, uint64_t exponent, uint8_t* dst, int dst_length)
{
    uint64_t exponent_field = significand_sign | (exponent_sign << 1) | (exponent << 2);
    KSLOG_TRACE("Exponent field = sig sign %d | exp sign %d << 1 | exp %d << 2 = 0x%x", significand_sign, exponent_sign, exponent, exponent_field);

    int offset = rvlq_encode_64(exponent_field, dst, dst_length);
    if(offset < 1)
    {
        return offset;
    }
    KSLOG_DATA_TRACE(dst, offset, "Encoded exp field 0x%x into", exponent_field);

    int bytes_encoded = rvlq_encode_64(significand, dst+offset, dst_length-offset);
    if(bytes_encoded < 1)
    {
        return bytes_encoded;
    }
    KSLOG_DATA_TRACE(dst+offset, bytes_encoded, "Encoded sig field 0x%lx into", significand);
    offset += bytes_encoded;

    KSLOG_DATA_TRACE(dst, offset, "Encoded data");
    return offset;
}

static void extract_float(uint64_t uvalue, int significant_digits, int* restrict exponent_ptr, uint64_t* restrict significand_ptr)
{
    uint64_t significand = 0;
    int exponent = 0;
    if((uvalue & MASK_EXTENDED) == VALUE_EXTENDED)
    {
        KSLOG_TRACE("Using extended VLQ");
        exponent = (uvalue >> SIZE_SIGNIFICAND_EXTENDED) & MASK_EXPONENT;
        significand = (uvalue & MASK_SIGNIFICAND_EXTENDED) | PREFIX_SIGNIFICAND_EXTENDED;
    }
    else
    {
        KSLOG_TRACE("Using normal VLQ");
        exponent = (uvalue >> SIZE_SIGNIFICAND_NORMAL) & MASK_EXPONENT;
        significand = uvalue & MASK_SIGNIFICAND_NORMAL;
    }
    exponent -= EXPONENT_BIAS;
    KSLOG_TRACE("Post-bias: exp %d, sig %d", exponent, significand);

    while(significand % 10 == 0)
    {
        significand /= 10;
        exponent++;
    }
    KSLOG_TRACE("Reduced exp %d (%x), sig %ld (%lx)", exponent, exponent, significand, significand);

    if(significant_digits > 0 && significant_digits < MAX_DIGITS_64_BIT)
    {
        int current_digits = 1;
        for(uint64_t sig_temp = significand / 10; sig_temp > 0; sig_temp /= 10)
        {
            current_digits++;
        }

        KSLOG_TRACE("Current digits %d", current_digits);
        while(current_digits > significant_digits)
        {
            unsigned remainder = significand % 10;
            significand /= 10;
            if(remainder > 5)
            {
                significand++;
            }
            else if(remainder == 5)
            {
                if(significand & 1)
                {
                    significand++;
                }
            }
            current_digits--;
            exponent++;
        }
        KSLOG_TRACE("Reduced to %d sig digits: %ld", significant_digits, significand);
    }

    *exponent_ptr = exponent;
    *significand_ptr = significand;
}


// ----------
// Public API
// ----------

const char* cfloat_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

ANSI_EXTENSION int cfloat_encoded_size(_Decimal64 value, int significant_digits)
{
    if(value == 0)
    {
        return 1;
    }

    uint64_t uvalue = 0;
    memcpy(&uvalue, &value, sizeof(uvalue));

    if(is_decimal_infinity(uvalue))
    {
        return 2;
    }

    if(is_decimal_nan(uvalue))
    {
        return 2;
    }

    uint64_t significand = 0;
    int exponent = 0;
    extract_float(uvalue, significant_digits, &exponent, &significand);

    uint64_t exponent_field = absolute_value(exponent) << 2;
    return rvlq_encoded_size_64(exponent_field) + rvlq_encoded_size_64(significand);
}

ANSI_EXTENSION int cfloat_encode(_Decimal64 dvalue, int significant_digits, uint8_t* dst, int dst_length)
{
    KSLOG_DEBUG("Encoding %f", (double)dvalue);

    uint64_t uvalue = 0;
    memcpy(&uvalue, &dvalue, sizeof(uvalue));
    KSLOG_TRACE("Raw value: %016lx", uvalue);

    if(dvalue == 0)
    {
        // Can't compare to -0.0dd because -0.0 is equal to 0.0
        return encode_zero(get_sign(uvalue), dst, dst_length);
    }

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
    extract_float(uvalue, significant_digits, &exponent, &significand);

    return encode_number(get_sign(uvalue), significand, get_sign(exponent), absolute_value(exponent), dst, dst_length);
}

ANSI_EXTENSION int cfloat_decode(const uint8_t* src, int src_length, _Decimal64* value)
{
    if(src_length < 1)
    {
        KSLOG_DEBUG("src_length %d is less than 1", src_length);
        return 0;
    }
    KSLOG_DATA_TRACE(src, src_length, "Decoding bytes");

    uint64_t exponent_field = 0;
    int offset = rvlq_decode_64(&exponent_field, src, src_length);
    if(offset < 1)
    {
        KSLOG_DATA_DEBUG(src, src_length, "Decode exponent failed, code %d:", offset);
        return offset;
    }
    KSLOG_TRACE("Decoded exponent field %x", exponent_field);

    KSLOG_TRACE("First byte = %02x", src[0]);
    if(vlq_is_extended(src)) // Extended RVLQ
    {
        KSLOG_TRACE("Encountered extended RVLQ");
        switch(exponent_field)
        {
            case ENCODED_VALUE_QUIET_NAN:
                KSLOG_TRACE("Encountered NaN, offset %d", offset);
                *value = NAN;
                return offset;
            case ENCODED_VALUE_SIGNALING_NAN:
                KSLOG_TRACE("Encountered siganling NaN, offset %d", offset);
                // TODO: Signaling NaN here
                *value = NAN;
                return offset;
            case ENCODED_VALUE_POS_INFINITY:
                KSLOG_TRACE("Encountered +infinity, offset %d", offset);
                *value = INFINITY;
                return offset;
            case ENCODED_VALUE_NEG_INFINITY:
                KSLOG_TRACE("Encountered -infinity, offset %d", offset);
                *value = -INFINITY;
                return offset;
        }
    }
    if(exponent_field == ENCODED_VALUE_POS_0)
    {
        *value = 0.0dd;
        return offset;
    }
    if(exponent_field == ENCODED_VALUE_NEG_0)
    {
        *value = -0.0dd;
        return offset;
    }

    uint64_t significand = 0;
    int bytesDecoded = rvlq_decode_64(&significand, src+offset, src_length-offset);
    if(bytesDecoded < 1)
    {
        KSLOG_DATA_DEBUG(src+offset, src_length-offset, "Decode significand failed, code %d:", bytesDecoded);
        return bytesDecoded;
    }
    offset += bytesDecoded;
    KSLOG_TRACE("Decoded significand %d", significand);

    uint64_t significand_sign = exponent_field & 1;
    unsigned exponent_sign = (exponent_field >> 1) & 1;
    uint64_t exponent_abs = exponent_field >> 2;

    if(significand > SIGNIFICAND_MAX)
    {
        KSLOG_DEBUG("Significand %ld is larger than max %ld", significand, SIGNIFICAND_MAX);
        return 0;
    }

    if(exponent_abs > EXPONENT_UNBIASED_MAX)
    {
        KSLOG_DEBUG("Exponent %d is larger than max %d", exponent_abs, EXPONENT_UNBIASED_MAX);
        return 0;
    }

    int exponent = exponent_abs;
    if(exponent_sign)
    {
        exponent = -exponent;
    }
    int64_t exponent_biased = exponent + EXPONENT_BIAS;
    KSLOG_TRACE("Exponent %d, biased %d (0x%x)", exponent, exponent_biased, exponent_biased);
    if(exponent_biased < 0)
    {
        KSLOG_DEBUG("Biased exponent %d is less than 0", exponent_biased);
        return 0;
    }
    if(exponent_biased > EXPONENT_BIASED_MAX)
    {
        KSLOG_DEBUG("Biased exponent %d is greater than max of %d", exponent_biased, EXPONENT_BIASED_MAX);
        return 0;
    }

    uint64_t uvalue = 0;
    if(significand & PREFIX_SIGNIFICAND_EXTENDED)
    {
        KSLOG_TRACE("Extended");
        uvalue = (significand_sign << 63) |
            VALUE_EXTENDED |
            (exponent_biased << SIZE_SIGNIFICAND_EXTENDED) | 
            (significand & ~PREFIX_SIGNIFICAND_EXTENDED);
    }
    else
    {
        KSLOG_TRACE("Not extended");
        uvalue = (significand_sign << 63) |
            (exponent_biased << SIZE_SIGNIFICAND_NORMAL) | 
            significand;
    }

    KSLOG_TRACE("Raw value: %016lx", uvalue);
    memcpy(value, &uvalue, sizeof(*value));
    KSLOG_TRACE("Decoded %d bytes", offset);
    return offset;
}
