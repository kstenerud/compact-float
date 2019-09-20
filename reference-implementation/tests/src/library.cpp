#include <gtest/gtest.h>
#include <compact_float/compact_float.h>
#include <cmath>

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cfloat_version();
    ASSERT_STREQ(expected, actual);
}


#define TEST_ENCODE_DECIMAL(NAME, VALUE, ...) \
TEST(CFloat, encode_decimal_ ## NAME) \
{ \
    _Decimal64 expected_value = VALUE; \
    std::vector<uint8_t> expected_data = __VA_ARGS__; \
    \
    int actual_size = cfloat_encoded_size_decimal(VALUE); \
    ASSERT_EQ(expected_data.size(), actual_size); \
    \
    std::vector<uint8_t> actual_data(10); \
    int bytes_encoded = cfloat_encode_decimal(expected_value, actual_data.data(), actual_data.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual_data.resize(bytes_encoded); \
    ASSERT_EQ(expected_data, actual_data); \
    \
    _Decimal64 actual_value = 0; \
    int bytes_decoded = cfloat_decode_decimal(actual_data.data(), bytes_encoded, &actual_value); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    ASSERT_EQ(actual_value, expected_value); \
}

#define TEST_ENCODE_BINARY(NAME, VALUE, DIGITS, EXPECTED_VALUE, ...) \
TEST(CFloat, encode_binary_ ## NAME ## _ ## DIGITS) \
{ \
    double value = VALUE; \
    double expected_value = EXPECTED_VALUE; \
    std::vector<uint8_t> expected_data = __VA_ARGS__; \
    std::vector<uint8_t> actual_data(10); \
    int bytes_encoded = cfloat_encode_binary(value, DIGITS, actual_data.data(), actual_data.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    actual_data.resize(bytes_encoded); \
    ASSERT_EQ(expected_data, actual_data); \
    \
    double actual_value = 0; \
    int bytes_decoded = cfloat_decode_binary(actual_data.data(), bytes_encoded, &actual_value); \
    ASSERT_EQ(bytes_encoded, bytes_decoded); \
    ASSERT_EQ(actual_value, expected_value); \
}

TEST_ENCODE_BINARY(0, 0.0, 0, 0, {0x02})
TEST_ENCODE_BINARY(n0, -0.0, 0, -0.0, {0x03})
TEST_ENCODE_BINARY(infinity, INFINITY, 0, INFINITY, {0x80, 0x02})
TEST_ENCODE_BINARY(n_infinity, -INFINITY, 0, -INFINITY, {0x80, 0x03})
// TEST_ENCODE_BINARY(nan, NAN, 0, NAN, {0x80, 0x00})

TEST_ENCODE_BINARY(0_2, 0.2, 4, 0.2, {0x06, 0x02})
TEST_ENCODE_BINARY(0_5935555, 0.5935555, 4, 0.5936, {0x12, 0xae, 0x30})


TEST_ENCODE_DECIMAL(0, 0.0dd, {0x02})
TEST_ENCODE_DECIMAL(n0, -0.0dd, {0x03})

TEST_ENCODE_DECIMAL(1_0, 1.0dd, {0x00, 0x01})
TEST_ENCODE_DECIMAL(1_5, 1.5dd, {0x06, 0x0f})
TEST_ENCODE_DECIMAL(1_2, 1.2dd, {0x06, 0x0c})
TEST_ENCODE_DECIMAL(1_25, 1.25dd, {0x0a, 0x7d})
TEST_ENCODE_DECIMAL(8_8419305, 8.8419305dd, {0x1e, 0xaa, 0x94, 0xd7, 0x69})
TEST_ENCODE_DECIMAL(9999999999999999, 9999999999999999.0dd, {0x00, 0x91, 0xe1, 0xde, 0xa6, 0xfe, 0x83, 0xff, 0x7f})
TEST_ENCODE_DECIMAL(9_3942e100, 9.3942e100dd, {0x83, 0x00, 0x85, 0xdd, 0x76})
TEST_ENCODE_DECIMAL(4_192745343en122, 4.192745343e-122dd, {0x84, 0x0e, 0x8f, 0xcf, 0xa0, 0xee, 0x7f})
