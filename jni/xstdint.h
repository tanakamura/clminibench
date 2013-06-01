#ifndef XSTDINT_H
#define XSTDINT_H

#if defined _MSC_VER && (_MSC_VER  < 1600)

typedef unsigned int uintptr_t;
typedef signed int intptr_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else
#include <stdint.h>
#endif

#endif
