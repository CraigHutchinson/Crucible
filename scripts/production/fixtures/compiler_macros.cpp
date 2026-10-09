#include <utils/Panic.h>

#if defined(_CPPUNWIND)
#pragma message("P15 _CPPUNWIND=1")
#else
#pragma message("P15 _CPPUNWIND=0")
#endif

#if defined(__EXCEPTIONS)
#pragma message("P15 __EXCEPTIONS=1")
#else
#pragma message("P15 __EXCEPTIONS=0")
#endif

#if UTILS_EXCEPTIONS
#pragma message("P15 UTILS_EXCEPTIONS=1")
#else
#pragma message("P15 UTILS_EXCEPTIONS=0")
#endif
