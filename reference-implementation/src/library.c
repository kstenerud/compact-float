#include "compact_float/compact_float.h"

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
