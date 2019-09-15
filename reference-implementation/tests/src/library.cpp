#include <gtest/gtest.h>
#include <compact_float/compact_float.h>
#include <cmath>

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cfloat_version();
    ASSERT_STREQ(expected, actual);
}

// TODO: Update for decimal
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



// TEST_ENCODE(_0, 0.0, {0x02})
// TEST_ENCODE(_n0, -0.0, {0x03})

// TEST_ENCODE(_1_0, 1.0, {0x00, 0x00})
// TEST_ENCODE(_1_5, 1.5, {0x00, 0x40})
// TEST_ENCODE(_1_25, 1.25, {0x00, 0x20})
