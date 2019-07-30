#include <gtest/gtest.h>
#include <cfloat/cfloat.h>

// #define KSLogger_LocalLevel DEBUG
#include "kslogger.h"

#define TEST_ENCODE(NAME, FLOAT_VALUE, ...) \
TEST(CFloat, encode_ ## NAME) \
{ \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(10); \
    int bytes_encoded = cfloat_encode(FLOAT_VALUE, actual.data(), actual.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual.resize(bytes_encoded); \
    ASSERT_EQ(expected, actual); \
    \
    double decoded; \
    int bytes_decoded = cfloat_decode(actual.data(), bytes_encoded, &decoded); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    ASSERT_EQ(decoded, FLOAT_VALUE); \
}



TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cfloat_version();
    ASSERT_STREQ(expected, actual);
}

TEST_ENCODE(_1_0, 1.0, {0x00, 0x00})
TEST_ENCODE(_1_5, 1.5, {0x00, 0x40})
TEST_ENCODE(_1_25, 1.25, {0x00, 0x20})

TEST_ENCODE(example_0_50830078125, 0.50830078125, {0x06, 0x90, 0x02})
TEST_ENCODE(example_n2_03703597633448608627ep90, -2.03703597633448608627e+90, {0x89, 0x31, 0x00})
