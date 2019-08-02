#include <math.h>

#include "compact_float/compact_float.h"
#include "vlq/vlq.h"

#define KSLogger_LocalLevel TRACE
#include "kslogger.h"

#define CFLOAT_EXPONENT_HIGH_BITS_COUNT 5
#define CFLOAT_EXPONENT_MAX_BEFORE_VLQ ((1<<CFLOAT_EXPONENT_HIGH_BITS_COUNT) - 1)

#define FLOAT64_BIN_BIAS 0X3FF
#define FLOAT64_BIN_WIDTH_EXPONENT 11
#define FLOAT64_BIN_WIDTH_SIGNIFICAND 52
#define FLOAT64_BIN_EXPONENT_ZERO_OR_SUBNORMAL 0
#define FLOAT64_BIN_EXPONENT_NAN_OR_INFINITY 0X7FF
#define FLOAT64_BIN_SHIFT_EXPONENT FLOAT64_BIN_WIDTH_SIGNIFICAND
#define FLOAT64_BIN_SHIFT_SIGN (FLOAT64_BIN_SHIFT_EXPONENT + FLOAT64_BIN_WIDTH_EXPONENT)
#define FLOAT64_BIN_MASK_EXPONENT ((1UL<<FLOAT64_BIN_WIDTH_EXPONENT) - 1)
#define FLOAT64_BIN_MASK_SIGNIFICAND ((1UL<<FLOAT64_BIN_WIDTH_SIGNIFICAND) - 1)

static inline void copy_64_bits(const uint8_t* restrict src, uint8_t* restrict dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[7] = src[7];
}

static inline uint64_t bin_float_to_bits_64(double value)
{
    uint64_t result = 0;
    copy_64_bits((uint8_t*)&value, (uint8_t*)&result);
    return result;
}

static inline double bits_to_bin_float_64(uint64_t bits)
{
    double result = 0;
    copy_64_bits((uint8_t*)&bits, (uint8_t*)&result);
    return result;
}

#define EXPONENT_IS_EXTENDED  0x8000000000000000UL
#define SIGNIFICAND_IS_PRESENT 0x4000000000000000UL
#define FLOAT_DATA_FLAGS_MASK 0xc000000000000000UL

static inline uint64_t get_float_data(double fvalue, uint64_t* significand_vlq)
{
    const uint64_t bits = bin_float_to_bits_64(fvalue);
    const uint64_t significand = bits & FLOAT64_BIN_MASK_SIGNIFICAND;
    const uint64_t exponent_raw = (bits >> FLOAT64_BIN_SHIFT_EXPONENT) & FLOAT64_BIN_MASK_EXPONENT;
    const uint64_t sign = bits >> FLOAT64_BIN_SHIFT_SIGN;
    int64_t exponent = (int64_t)exponent_raw - FLOAT64_BIN_BIAS;
    uint64_t exponent_sign = 0;
    if(exponent < 0)
    {
        exponent = -exponent;
        exponent_sign = 1;
    }

    uint64_t data_flags = SIGNIFICAND_IS_PRESENT;

    if(exponent_raw == FLOAT64_BIN_EXPONENT_ZERO_OR_SUBNORMAL)
    {
        if(significand == 0)
        {
            // +- 0 is encoded as exponent value -0, no significand
            exponent_sign = 1;
            exponent = 0;
            data_flags &= ~SIGNIFICAND_IS_PRESENT;
        }
        else
        {
            // Subnormal is encoded with extended exponent
            data_flags |= EXPONENT_IS_EXTENDED;
        }
    }
    else if(exponent_raw == FLOAT64_BIN_EXPONENT_NAN_OR_INFINITY)
    {
        data_flags |= EXPONENT_IS_EXTENDED;

        if(significand == 0)
        {
            // +- infinity is encoded as extended exponent value 0, no significand
            exponent_sign = 0;
            exponent = 0;
            data_flags &= ~SIGNIFICAND_IS_PRESENT;
        }
        else
        {
            // NaN is encoded as extended exponent value -0, payload in significand
            exponent_sign = 1;
            exponent = 0;
        }
    }

    uint64_t exponent_vlq = sign | (exponent_sign << 1) | (exponent << 2);
    *significand_vlq = significand << (FLOAT64_BIN_WIDTH_EXPONENT + 1);

    return exponent_vlq | data_flags;
}


// ----------
// Public API
// ----------

int cfloat_binary_encoded_size(double value)
{
    uint64_t significand_vlq = 0;
    const uint64_t data = get_float_data(value, &significand_vlq);
    const uint64_t exponent_vlq = data & ~FLOAT_DATA_FLAGS_MASK;
    const int base_length = (data & EXPONENT_IS_EXTENDED) ? 1 : 0;
    const int significand_length = (data & SIGNIFICAND_IS_PRESENT) ? lvlq_encoded_size_64(significand_vlq) : 0;

    return base_length + rvlq_encoded_size_64(exponent_vlq) + significand_length;
}

int cfloat_binary_encode(double fvalue, uint8_t* dst, int dst_length)
{
    if(dst_length < 1)
    {
        return 0;
    }

    uint64_t significand_vlq = 0;
    const uint64_t data = get_float_data(fvalue, &significand_vlq);
    const uint64_t exponent_vlq = data & ~FLOAT_DATA_FLAGS_MASK;
    int base_length = 0;

    if(data & EXPONENT_IS_EXTENDED)
    {
        *dst++ = 0x80;
        dst_length--;
        base_length++;
    }

    int exponent_length = rvlq_encode_64(exponent_vlq, dst, dst_length);
    if(exponent_length < 1)
    {
        return exponent_length;
    }
    int significand_length = 0;

    dst += exponent_length;
    dst_length -= exponent_length;
    if(data & SIGNIFICAND_IS_PRESENT)
    {
        significand_length = lvlq_encode_64(significand_vlq, dst, dst_length);
        if(significand_length < 1)
        {
            return significand_length;
        }
    }

    return base_length + exponent_length + significand_length;
}

int cfloat_binary_decode(const uint8_t* src, int src_length, double* value)
{
    if(src_length == 0)
    {
        return 0;
    }
    if(*src == 2)
    {
        *value = 0.0;
        return 1;
    }
    if(*src == 3)
    {
        *value = -0.0;
        return 1;
    }
    bool is_extended = *src == 0x80;

    uint64_t exponent_vlq = 0;
    int exponent_length = rvlq_decode_64(&exponent_vlq, src, src_length);
    if(exponent_length == 0)
    {
        return 0;
    }
    if(is_extended)
    {
        if(exponent_vlq == 0)
        {
            *value = INFINITY;
            return exponent_length;
        }
        if(exponent_vlq == 1)
        {
            *value = -INFINITY;
            return exponent_length;
        }
    }
    uint64_t significand_vlq = 0;
    int significand_length = lvlq_decode_64(&significand_vlq, src + exponent_length, src_length - exponent_length);
    if(significand_length == 0)
    {
        return 0;
    }

    uint64_t significand = significand_vlq >> (FLOAT64_BIN_WIDTH_EXPONENT + 1);
    uint64_t sign = exponent_vlq & 1;
    uint64_t exponent_sign = (exponent_vlq>>1) & 1;
    int64_t exponent = (int64_t)(exponent_vlq >> 2);
    if(is_extended)
    {
        if(exponent == 0 && exponent_sign == 1)
        {
            exponent = FLOAT64_BIN_EXPONENT_NAN_OR_INFINITY;
        }
        // TODO: Subnormals from different sized source?
    }
    else
    {
        if(exponent_sign != 0)
        {
            exponent = -exponent;
        }
        exponent += FLOAT64_BIN_BIAS;
    }

    uint64_t bits = significand | (exponent << FLOAT64_BIN_SHIFT_EXPONENT) | (sign << FLOAT64_BIN_SHIFT_SIGN);
    copy_64_bits((uint8_t*)&bits, (uint8_t*)value);

    return exponent_length + significand_length;
}
