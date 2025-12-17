#ifndef LOCAL_MATH_H
#define LOCAL_MATH_H

#include <stddef.h>



static inline int min_int(int a, int b) {
    return a < b ? a : b;
}

static inline size_t min_size_t(size_t a, size_t b) {
    return a < b ? a : b;
}

static inline uint32_t min_uint32_t(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

#define min(a, b) _Generic ((a), \
        int: min_int, \
        size_t: min_size_t, \
        uint32_t: min_uint32_t \
    ) (a, b)



static inline int max_int(int a, int b) {
    return a > b ? a : b;
}

static inline size_t max_size_t(size_t a, size_t b) {
    return a > b ? a : b;
}

#define max(a, b) _Generic ((a), \
        int: max_int, \
        size_t: max_size_t \
    ) (a, b)



static inline int local_abs_int(int num) {
    return num < 0 ? -num : num;
}

#define local_abs(a, b) _Generic ((a), \
        int: local_abs_int, \
    ) (a, b)



static inline size_t get_next_multiple(size_t num, size_t divisor) {
    return num + (divisor - num%divisor)%divisor;
}

#endif // LOCAL_MATH_H
