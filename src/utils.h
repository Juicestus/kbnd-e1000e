#ifndef KBND_UTILS_H_
#define KBND_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <time.h>

/**
 * Handy macros
 */

#define UNUSED(VAR)     (void)(VAR)
#define ARRAY_LEN(ARR)  (sizeof(ARR)/sizeof(ARR_LEN))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(a, x, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) : (x))

#ifndef NULL
    #define NULL ((void *)0)
    typedef unsigned long long size_t;
#endif

/**
 * usleep that wraps nanosleep
 */
void usleep2(uint64_t us);

#endif
