#include <math.h>

#include "cfloat/cfloat.h"

// #define KSLogger_LocalLevel TRACE
#include "kslogger.h"

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

// ----------
// Public API
// ----------

const char* cfloat_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

int cfloat_encode_binaryd_size(double value)
{
    (void)value;
    return 0;
}

