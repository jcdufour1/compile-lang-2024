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

#define string_print(string) strv_print(string_to_strv(string))

static inline void string_extend_cstr(Arena* arena, String* str, const char* cstr) {
    for (;*cstr; cstr++) {
        vec_append(arena, str, *cstr);
    }
}

static inline void string_extend_hex_8_digits(Arena* arena, String* str, uint32_t num) {
    char num_str[9] = {0};
    sprintf(num_str, "%08x", num);
    string_extend_cstr(arena, str, num_str);
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

static inline void string_extend_int64_t(Arena* arena, String* str, int64_t num) {
    char num_str[32] = {0};
    sprintf(num_str, "%"PRId64, num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_uint64_t(Arena* arena, String* str, uint64_t num) {
    char num_str[32] = {0};
    sprintf(num_str, "%"PRIu64, num);
    string_extend_cstr(arena, str, num_str);
}

static inline void string_extend_uint16_t(Arena* arena, String* str, uint16_t num) {
    string_extend_uint64_t(arena, str, (uint64_t)num);
}

static inline void string_extend_int16_t(Arena* arena, String* str, int16_t num) {
    string_extend_int64_t(arena, str, (int64_t)num);
}

static inline void string_extend_int(Arena* arena, String* str, int num) {
    string_extend_uint64_t(arena, str, (uint64_t)num);
}

static inline void string_extend_pointer(Arena* arena, String* str, const void* pointer) {
    char num_str[32] = {0};
    sprintf(num_str, "%p", pointer);
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

static inline void string_extend_strv_indent(Arena* arena, String* string, Strv strv, Indent indent) {
    for (size_t idx = 0; idx < indent; idx++) {
        vec_append(arena, string, '-');
    }
    string_extend_strv(arena, string, strv);
}

static inline void string_extend_cstr_indent(Arena* arena, String* string, const char* cstr, Indent indent) {
    string_extend_strv_indent(arena, string, sv(cstr), indent);
}

static inline void string_extend_line(Arena* arena, String* string, uint32_t num) {
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
    return strv_dup(arena, string_to_strv(string));
}

static inline Strv string_slice(String string, size_t start, size_t count) {
    unwrap(start < string.info.count && "out of bounds");
    unwrap(start + count <= string.info.count && "out of bounds");
    return (Strv) {.str = string.buf + start, .count = count};
}

// TODO: move these functions to newstring.h?
static void string_extend_upper(Arena* arena, String* string, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        vec_append(arena, string, toupper(strv_at(strv, idx)));
    }
}

static inline void string_extend_first_upper(Arena* arena, String* string, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        if (idx == 0) {
            vec_append(arena, string, toupper(strv_at(strv, idx)));
        } else {
            vec_append(arena, string, tolower(strv_at(strv, idx)));
        }
    }
}

static inline void string_extend_lower(Arena* arena, String* string, Strv strv) {
    for (size_t idx = 0; idx < strv.count; idx++) {
        vec_append(arena, string, tolower(strv_at(strv, idx)));
    }
}

// TODO: rename these 3 below functions?
static inline Strv strv_lower_print_internal(Arena* arena, Strv strv) {
    String buf = {0};
    string_extend_lower(arena, &buf, strv);
    return string_to_strv(buf);
}

static inline Strv strv_upper_print_internal(Arena* arena, Strv strv) {
    String buf = {0};
    string_extend_upper(arena, &buf, strv);
    return string_to_strv(buf);
}

static inline Strv strv_first_upper_print_internal(Arena* arena, Strv strv) {
    String buf = {0};
    string_extend_first_upper(arena, &buf, strv);
    return string_to_strv(buf);
}

#define strv_lower_print(arena, strv) \
    strv_print(strv_lower_print_internal(arena, strv))

#define strv_first_upper_print(arena, strv) \
    strv_print(strv_first_upper_print_internal(arena, strv))

#define strv_upper_print(arena, strv) \
    strv_print(strv_upper_print_internal(arena, strv))

__attribute__((format (printf, 3, 4)))
void string_extend_f(Arena* arena, String* string, const char* format, ...);

__attribute__((format (printf, 2, 3)))
Strv strv_from_f(Arena* arena, const char* format, ...);

#endif // NEWSTRING_H
