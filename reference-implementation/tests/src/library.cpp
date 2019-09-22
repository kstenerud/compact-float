#include <gtest/gtest.h>
#include <compact_float/compact_float.h>
#include <cmath>

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cfloat_version();
    ASSERT_STREQ(expected, actual);
}


#define TEST_ENCODE(NAME, VALUE, DIGITS, EXPECTED_VALUE, ...) \
TEST(CFloat, encode_ ## NAME) \
{ \
    _Decimal64 value = VALUE; \
    _Decimal64 expected_value = EXPECTED_VALUE; \
    std::vector<uint8_t> expected_data = __VA_ARGS__; \
    \
    int actual_size = cfloat_encoded_size(VALUE, DIGITS); \
    ASSERT_EQ(expected_data.size(), actual_size); \
    \
    std::vector<uint8_t> actual_data(15); \
    int bytes_encoded = cfloat_encode(value, DIGITS, actual_data.data(), actual_data.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual_data.resize(bytes_encoded); \
    ASSERT_EQ(expected_data, actual_data); \
    \
    _Decimal64 actual_value = 0; \
    int bytes_decoded = cfloat_decode(actual_data.data(), bytes_encoded, &actual_value); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    ASSERT_EQ(actual_value, expected_value); \
}


// TEST_ENCODE(nan, NAN, 0, NAN, {0x80, 0x00})
TEST_ENCODE(infinity, INFINITY, 0, INFINITY, {0x80, 0x02})
TEST_ENCODE(n_infinity, -INFINITY, 0, -INFINITY, {0x80, 0x03})
TEST_ENCODE(0, 0.0dd, 0, 0.0dd, {0x02})
TEST_ENCODE(n0, -0.0dd, 0, -0.0dd, {0x03})

TEST_ENCODE(1_0, 1.0dd, 0, 1.0dd, {0x00, 0x01})
TEST_ENCODE(1_5, 1.5dd, 0, 1.5dd, {0x06, 0x0f})
TEST_ENCODE(1_2, 1.2dd, 0, 1.2dd, {0x06, 0x0c})
TEST_ENCODE(1_25, 1.25dd, 0, 1.25dd, {0x0a, 0x7d})
TEST_ENCODE(8_8419305, 8.8419305dd, 0, 8.8419305dd, {0x1e, 0xaa, 0x94, 0xd7, 0x69})

TEST_ENCODE(1999999999999999, 1999999999999999.0dd, 0, 1999999999999999.0dd, {0x00, 0x83, 0xc6, 0xdf, 0xd4, 0xcc, 0xb3, 0xff, 0x7f})
TEST_ENCODE(9_3942e100, 9.3942e100dd, 0, 9.3942e100dd, {0x83, 0x00, 0x85, 0xdd, 0x76})
TEST_ENCODE(4_192745343en122, 4.192745343e-122dd, 0, 4.192745343e-122dd, {0x84, 0x0e, 0x8f, 0xcf, 0xa0, 0xee, 0x7f})

TEST_ENCODE(bin_0_2, 0.2, 4, 0.2, {0x06, 0x02})
TEST_ENCODE(bin_0_5935555, 0.5935555, 4, 0.5936, {0x12, 0xae, 0x30})
