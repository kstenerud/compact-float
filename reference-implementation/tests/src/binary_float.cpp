#include <gtest/gtest.h>
#include <compact_float/compact_float.h>
#include <cmath>

// #define KSLogger_LocalLevel DEBUG
#include "kslogger.h"

#define TEST_ENCODE(NAME, FLOAT_VALUE, ...) \
TEST(CFloat, encode_ ## NAME) \
{ \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(10); \
    int actual_size = cfloat_binary_encoded_size(FLOAT_VALUE); \
    ASSERT_EQ(expected.size(), actual_size); \
    int bytes_encoded = cfloat_binary_encode(FLOAT_VALUE, actual.data(), actual.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual.resize(bytes_encoded); \
    ASSERT_EQ(expected, actual); \
    \
    double decoded; \
    int bytes_decoded = cfloat_binary_decode(actual.data(), bytes_encoded, &decoded); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    ASSERT_EQ(decoded, FLOAT_VALUE); \
}

#define TEST_ENCODE_NAN(NAME, FLOAT_VALUE, ...) \
TEST(CFloat, encode_ ## NAME) \
{ \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(10); \
    int actual_size = cfloat_binary_encoded_size(FLOAT_VALUE); \
    ASSERT_EQ(expected.size(), actual_size); \
    int bytes_encoded = cfloat_binary_encode(FLOAT_VALUE, actual.data(), actual.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual.resize(bytes_encoded); \
    ASSERT_EQ(expected, actual); \
    \
    double decoded; \
    int bytes_decoded = cfloat_binary_decode(actual.data(), bytes_encoded, &decoded); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    uint64_t udecoded = 0; \
    memcpy((char*)&udecoded, (char*)&decoded, sizeof(decoded)); \
    double dvalue = FLOAT_VALUE; \
    uint64_t uvalue = 0; \
    memcpy((char*)&uvalue, (char*)&dvalue, sizeof(dvalue)); \
    ASSERT_EQ(uvalue, udecoded); \
}

static double nan_with_payload(uint64_t payload)
{
    const uint64_t mask = ((uint64_t)1<<52)-1;
    double result = NAN;
    uint64_t uvalue = 0;
    memcpy((char*)&uvalue, (char*)&result, sizeof(result));
    uvalue = (uvalue & ~mask) | (payload & mask);
    memcpy((char*)&result, (char*)&uvalue, sizeof(result));
    return result;
}


// TEST_ENCODE(_0, 0.0, {0x02})
// TEST_ENCODE(_n0, -0.0, {0x03})

// TEST_ENCODE(_1_0, 1.0, {0x00, 0x00})
// TEST_ENCODE(_1_5, 1.5, {0x00, 0x40})
// TEST_ENCODE(_1_25, 1.25, {0x00, 0x20})

// TEST_ENCODE(example_0_50830078125, 0.50830078125, {0x06, 0x90, 0x02})
// TEST_ENCODE(example_n2_03703597633448608627ep90, -2.03703597633448608627e+90, {0x89, 0x31, 0x00})

// TEST_ENCODE(inf, INFINITY, {0x80, 0x00})
// TEST_ENCODE(ninf, -INFINITY, {0x80, 0x01})

// TEST_ENCODE_NAN(nan_1, nan_with_payload(1), {0x80, 0x02, 0x90, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00})
// TEST_ENCODE_NAN(nan_x10000000, nan_with_payload(0x10000000), {0x80, 0x02, 0x90, 0x80, 0x80, 0x00})
