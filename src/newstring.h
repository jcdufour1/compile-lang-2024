#ifndef NEWSTRING_H
#define NEWSTRING_H

#include <string.h>
#include "util.h"
#include "str_view.h"
#include "vector.h"
#include "assert.h"
#include <inttypes.h>
#include <stdint.h>

#define STRING_DEFAULT_CAPACITY 1

typedef struct {
    Vec_base info;
    char* buf;
} String;

#define STRING_FMT "%.*s"

#define string_print(string) (int)((string).info.count), (string).buf

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
    char num_str[21];
    sprintf(num_str, "%zu", num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_int64_t(Arena* arena, String* str, int64_t num) {
    char num_str[21];
    sprintf(num_str, "%"PRId64, num);
    string_extend_cstr(arena, str, num_str);
}

static inline String string_new_from_cstr(Arena* arena, const char* cstr) {
    String string = {0};
    for (size_t idx = 0; cstr[idx]; idx++) {
        vec_append(arena, &string, cstr[idx]);
    }
    return string;
}

static inline String string_new_from_strv(Arena* arena, Str_view str_view) {
    String string = {0};
    for (size_t idx = 0; str_view.str[idx]; idx++) {
        vec_append(arena, &string, str_view.str[idx]);
    }
    return string;
}

static inline void string_extend_strv(Arena* arena, String* string, Str_view str_view) {
    for (size_t idx = 0; idx < str_view.count; idx++) {
        vec_append(arena, string, str_view.str[idx]);
    }
}

static inline void string_add_int(Arena* arena, String* string, int num) {
    const char* fmt_str =  " (line %d)";
    static char num_str[20];
    sprintf(num_str, fmt_str, num);
    string_extend_cstr(arena, string, num_str);
}

static inline void string_extend_strv_in_sym(Arena* arena, String* string, Str_view str_view, char opening_symbol, char closing_symbol) {
    vec_append(arena, string, opening_symbol);
    string_extend_strv(arena, string, str_view);
    vec_append(arena, string, closing_symbol);
}

static inline void string_extend_strv_in_par(Arena* arena, String* string, Str_view str_view) {
    string_extend_strv_in_sym(arena, string, str_view, '(', ')');
}

static inline void string_extend_strv_in_gtlt(Arena* arena, String* string, Str_view str_view) {
    string_extend_strv_in_sym(arena, string, str_view, '<', '>');
}

static inline Str_view string_to_strv(const String string) {
    Str_view str_view = {.str = string.buf, .count = string.info.count};
    return str_view;
}

#endif // NEWSTRING_H
