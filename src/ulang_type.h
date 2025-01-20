#ifndef ULANG_TYPE_H
#define ULANG_TYPE_H

typedef struct {
    Str_view str;
    int16_t pointer_depth;
} ULang_type;

typedef struct {
    Vec_base info;
    ULang_type* buf;
} ULang_type_vec;

static inline ULang_type ulang_type_new(Str_view str, int16_t pointer_depth) {
    return (ULang_type) {.str = str, .pointer_depth = pointer_depth};
}

static inline ULang_type ulang_type_new_from_cstr(const char* cstr, int16_t pointer_depth) {
    return ulang_type_new(str_view_from_cstr(cstr), pointer_depth);
}

#endif // ULANG_TYPE_H
