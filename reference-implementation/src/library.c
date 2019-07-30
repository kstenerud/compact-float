#include "cfloat/cfloat.h"
#include "vlq/vlq.h"

#define KSLogger_LocalLevel TRACE
#include "kslogger.h"

#define CFLOAT_EXPONENT_HIGH_BITS_COUNT 5
#define CFLOAT_EXPONENT_MAX_BEFORE_VLQ ((1<<CFLOAT_EXPONENT_HIGH_BITS_COUNT) - 1)

#define FLOAT64_BIAS 0X3FF
#define FLOAT64_WIDTH_EXPONENT 11
#define FLOAT64_WIDTH_SIGNIFICAND 52
#define FLOAT64_EXPONENT_ZERO_OR_SUBNORMAL 0
#define FLOAT64_EXPONENT_NAN 0X7FF
#define FLOAT64_SHIFT_EXPONENT FLOAT64_WIDTH_SIGNIFICAND
#define FLOAT64_SHIFT_SIGN (FLOAT64_SHIFT_EXPONENT + FLOAT64_WIDTH_EXPONENT)
#define FLOAT64_MASK_EXPONENT ((1UL<<FLOAT64_WIDTH_EXPONENT) - 1)
#define FLOAT64_MASK_SIGNIFICAND ((1UL<<FLOAT64_WIDTH_SIGNIFICAND) - 1)

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

// ----------
// Public API
// ----------

const char* cfloat_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

int cfloat_encoded_size(double value)
{
    (void)value;
    return 0;
}

int cfloat_encode(double fvalue, uint8_t* dst, int dst_length)
{
    uint64_t uvalue = 0;
    memcpy((uint8_t*)&uvalue, (uint8_t*)&fvalue, sizeof(uvalue));
    uint64_t significand = uvalue & FLOAT64_MASK_SIGNIFICAND;
    int64_t exponent = (uvalue >> FLOAT64_SHIFT_EXPONENT) & FLOAT64_MASK_EXPONENT;
    uint64_t sign = uvalue >> FLOAT64_SHIFT_SIGN;

    if(exponent == FLOAT64_EXPONENT_NAN)
    {
        // TODO
        return 0;
    }

    if(exponent == FLOAT64_EXPONENT_ZERO_OR_SUBNORMAL)
    {
        // TODO
        return 0;
    }

    exponent -= FLOAT64_BIAS;
    uint64_t exponent_sign = 0;
    if(exponent < 0)
    {
        exponent = -exponent;
        exponent_sign = 1;
    }
    uint64_t exponent_vlq = sign |
                            (exponent_sign << 1) |
                            (exponent << 2);

    uint64_t significand_vlq = significand << (FLOAT64_WIDTH_EXPONENT + 1);

    int exponent_length = rvlq_encode_64(exponent_vlq, dst, dst_length);
    int significand_length = lvlq_encode_64(significand_vlq, dst+exponent_length, dst_length - exponent_length);
    if(exponent_length == 0 || significand_length == 0)
    {
        return 0;
    }
    return exponent_length + significand_length;
}

int cfloat_decode(const uint8_t* src, int src_length, double* value)
{
    uint64_t exponent_vlq = 0;
    uint64_t significand_vlq = 0;
    int exponent_length = rvlq_decode_64(&exponent_vlq, src, src_length);
    int significand_length = lvlq_decode_64(&significand_vlq, src + exponent_length, src_length - exponent_length);
    if(exponent_length == 0 || significand_length == 0)
    {
        return 0;
    }
    uint64_t significand = significand_vlq >> (FLOAT64_WIDTH_EXPONENT + 1);
    uint64_t sign = exponent_vlq & 1;
    uint64_t exponent_sign = (exponent_vlq>>1) & 1;
    int64_t exponent = (int64_t)(exponent_vlq >> 2);
    if(exponent_sign != 0)
    {
        exponent = -exponent;
    }
    exponent += FLOAT64_BIAS;

    uint64_t uvalue = significand | (exponent << FLOAT64_SHIFT_EXPONENT) | (sign << FLOAT64_SHIFT_SIGN);
    memcpy((uint8_t*)value, (uint8_t*)&uvalue, sizeof(*value));

    return exponent_length + significand_length;
}
