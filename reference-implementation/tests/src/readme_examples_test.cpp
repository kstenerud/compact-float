#include <gtest/gtest.h>
#include <stdio.h>
#include <vector>
#include <compact_float/compact_float.h>

static void print_buffer(const uint8_t* data, int length)
{
    for(int i = 0; i < length; i++)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

static void demonstrate_encode()
{
    double value = -3.23828125;
    int size = cfloat_binary_encoded_size(value);
    std::vector<uint8_t> data(size);
    int encoded_count = cfloat_binary_encode(value, data.data(), data.size());
    if(encoded_count <= 0)
    {
        // TODO: Error
    }
    print_buffer(data.data(), data.size()); // Prints 05 a0 4f
}

static void demonstrate_decode()
{
    std::vector<uint8_t> data = {0x04, 0xc0, 0xb0, 0x0f};
    double value = 0;
    int decoded_count = cfloat_binary_decode(data.data(), data.size(), &value);
    if(decoded_count <= 0)
    {
        // TODO: Error
    }
    printf("%.20g\n", value); // Prints 2.24029541015625
}

TEST(Readme, example)
{
	demonstrate_encode();
    demonstrate_decode();
}
