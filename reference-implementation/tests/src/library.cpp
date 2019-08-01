#include <gtest/gtest.h>
#include <cfloat/cfloat.h>

// #define KSLogger_LocalLevel DEBUG
#include "kslogger.h"

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cfloat_version();
    ASSERT_STREQ(expected, actual);
}
