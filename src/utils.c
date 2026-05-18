#include "utils.h"

/**
 * Sleep for `us` microseconds.
 */
 void usleep2(uint64_t us)
{
    struct timespec ts = {
        .tv_sec  = us / 1000000,
        .tv_nsec = (us % 1000000) * 1000,
    };
    nanosleep(&ts, NULL);
}
