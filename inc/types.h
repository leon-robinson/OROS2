#pragma once

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#ifndef __cplusplus
typedef unsigned char bool;
#define true 1
#define false 0
#endif

#define NULL ((void *)0)
#define PACKED __attribute__((packed))
#define ALWAYS_INLINE __attribute__((always_inline))
#define NO_RETURN __attribute__((no_return))