#ifndef UINT8_T_DARR_H
#define UINT8_T_DARR_H

#include <util.h>
#include <strv.h>

typedef struct {
    Vec_base info;
    uint8_t* buf;
} Uint8_t_darr;

// TODO: move *view struct and functions to separate file?
typedef struct {
    uint8_t* buf;
    size_t count;
} Uint8_t_view;

static inline Uint8_t_view uint8_t_view_new(uint8_t* buf, size_t count) {
    return (Uint8_t_view) {.buf = buf, .count = count};
}

static Strv uint8_t_view_print_internal(Uint8_t_view view) {
    String buf = {0};

    for (size_t idx = 0; idx < view.count; idx++) {
        string_extend_f(&a_temp, &buf, "%02X", view.buf[idx]);
    }

    return string_to_strv(buf);
}

static inline Bytes uint8_t_view_to_bytes(Uint8_t_view view) {
    assert(view.count == sizeof(Bytes));

    Bytes new_bytes = {0};
    memcpy(&new_bytes, view.buf, sizeof(Bytes));
    return new_bytes;
}

static inline Uint8_t_view uint8_t_view_from_uint64_t(uint64_t* num) {
    return uint8_t_view_new((uint8_t*)num, sizeof(*num));
}

static inline uint64_t uint8_t_view_cast_to_uint64_t(Uint8_t_view view) {
    assert(view.count <= sizeof(uint64_t));

    uint64_t new_int = {0};
    memcpy(&new_int, view.buf, view.count);
    return new_int;
}

#define uint8_t_view_print(view) strv_print(uint8_t_view_print_internal(view))

#endif // UINT8_T_DARR_H
