#ifndef LOCAL_MATH_H
#define LOCAL_MATH_H

#include <stddef.h>

static inline uint8_t min_uint8_t(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

static inline uint16_t min_uint16_t(uint16_t a, uint16_t b) {
    return a < b ? a : b;
}

static inline uint32_t min_uint32_t(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

static inline uint64_t min_uint64_t(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

#define min(a, b) _Generic ((a), \
        uint8_t: min_uint8_t, \
        uint16_t: min_uint16_t, \
        uint32_t: min_uint32_t, \
        uint64_t: min_uint64_t \
    ) (a, b)



static inline uint64_t max_uint64_t(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

static inline uint32_t max_uint32_t(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

static inline uint16_t max_uint16_t(uint16_t a, uint16_t b) {
    return a > b ? a : b;
}

static inline uint8_t max_uint8_t(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

#define max(a, b) _Generic ((a), \
        uint8_t: max_uint8_t, \
        uint16_t: max_uint16_t, \
        uint32_t: max_uint32_t, \
        uint64_t: max_uint64_t, \
        Bytes: bytes_max \
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
