#ifndef NEWSTRING_H
#define NEWSTRING_H

#include <string.h>
#include <util.h>
#include <strv.h>
#include <vector.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>

#define STRING_DEFAULT_CAPACITY 1

typedef struct {
    Vec_base info;
    char* buf;
} String;

#define STRING_FMT STR_VIEW_FMT

#define string_print(string) strv_print(string_to_strv(string))

static inline void string_extend_cstr(Arena* arena, String* str, const char* cstr) {
    for (;*cstr; cstr++) {
        vec_append(arena, str, *cstr);
    }
}

static inline void string_extend_hex_2_digits(Arena* arena, String* str, uint8_t num) {
    char num_str[3];
    sprintf(num_str, "%02x", num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_size_t(Arena* arena, String* str, size_t num) {
    char num_str[32] = {0};
    sprintf(num_str, "%zu", num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_pointer(Arena* arena, String* str, const void* pointer) {
    char num_str[32] = {0};
    sprintf(num_str, "%p", pointer);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_int64_t(Arena* arena, String* str, int64_t num) {
    char num_str[32];
    sprintf(num_str, "%"PRId64, num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_double(Arena* arena, String* str, double num) {
    const size_t BUF_SIZE = 32;
    char num_str[BUF_SIZE];
    int actual = snprintf(NULL, 0, "%.17f", num);
    if (actual < 1) {
        // error occured
        todo();
    }
    if ((size_t)actual >= BUF_SIZE) {
        unreachable("buffer will overflow when writing float");
    }

    unwrap(actual == snprintf(num_str, BUF_SIZE, "%.17f", num));
    string_extend_cstr(arena, str, num_str);
}

static inline void string_append_character(Arena* arena, String* str, uint8_t num) {
    vec_append(arena, str, num);
}

static inline String string_new_from_cstr(Arena* arena, const char* cstr) {
    String string = {0};
    for (size_t idx = 0; cstr[idx]; idx++) {
        vec_append(arena, &string, cstr[idx]);
    }
    return string;
}

static inline String string_new_from_strv(Arena* arena, Strv strv) {
    String string = {0};
    for (size_t idx = 0; idx < strv.count; idx++) {
        vec_append(arena, &string, strv_at(strv, idx));
    }
    return string;
}

static inline void string_extend_strv(Arena* arena, String* string, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        vec_append(arena, string, strv_at(strv, idx));
    }
}

static inline void string_extend_strv_indent(Arena* arena, String* string, Strv strv, size_t indent) {
    for (size_t idx = 0; idx < indent; idx++) {
        // TODO: should this always be done this way?
        vec_append(arena, string, '-');
    }
    string_extend_strv(arena, string, strv);
}

static inline void string_extend_cstr_indent(Arena* arena, String* string, const char* cstr, size_t indent) {
    string_extend_strv_indent(arena, string, sv(cstr), indent);
}

static inline void string_add_int(Arena* arena, String* string, int num) {
    const char* fmt_str =  " (line %d)";
    static char num_str[200];
    sprintf(num_str, fmt_str, num);
    string_extend_cstr(arena, string, num_str);
}

static inline void string_extend_strv_in_sym(Arena* arena, String* string, Strv strv, char opening_symbol, char closing_symbol) {
    vec_append(arena, string, opening_symbol);
    string_extend_strv(arena, string, strv);
    vec_append(arena, string, closing_symbol);
}

static inline void string_extend_strv_in_par(Arena* arena, String* string, Strv strv) {
    string_extend_strv_in_sym(arena, string, strv, '(', ')');
}

static inline void string_extend_strv_in_gtlt(Arena* arena, String* string, Strv strv) {
    string_extend_strv_in_sym(arena, string, strv, '<', '>');
}

static inline Strv string_to_strv(const String string) {
    Strv strv = {.str = string.buf, .count = string.info.count};
    return strv;
}

static inline String string_clone(Arena* new_arena, String string) {
    String new_str = {0};
    string_extend_strv(new_arena, &new_str, string_to_strv(string));
    return new_str;
}

static inline const char* string_to_cstr(Arena* arena, String string) {
    return strv_to_cstr(arena, string_to_strv(string));
}

#endif // NEWSTRING_H
