#pragma once

#include "common/panic.h"
#include "lib/macros.h"

#ifdef __BUILD_RELEASE
#define ASSERT(EXPR)
#else
#define ASSERT(EXPR)                                                                                     \
    ({                                                                                                   \
        if(!(EXPR)) panic("Assertion \"%s\" failed in " __FILE__ ":" MACROS_STRINGIFY(__LINE__), #EXPR); \
    })
#endif
