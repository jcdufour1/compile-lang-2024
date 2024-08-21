#ifndef NEWSTRING_H
#define NEWSTRING_H

#include <string.h>
#include "util.h"
#include "str_view.h"
#include "vector.h"
#include "assert.h"

#define STRING_DEFAULT_CAPACITY 8

typedef struct {
    Vec_base info;
    char* buf;
} String;

#define STRING_FMT "%.*s"

#define string_print(string) (int)((string).info.count), (string).buf

static inline void string_reserve(String* str, size_t minimum_count_empty_slots) {
    vector_reserve(str, sizeof(str->buf[0]), minimum_count_empty_slots + 1, STRING_DEFAULT_CAPACITY);
}

// string->buf is always null terminated
static inline void string_append(String* str, char ch) {
    assert(sizeof(str->buf[0]) == 1);
    vector_append(str, sizeof(str->buf[0]), &ch, STRING_DEFAULT_CAPACITY);
}

// string->buf is always null terminated
static inline void string_extend_cstr(String* str, const char* cstr) {
    for (;*cstr; cstr++) {
        string_append(str, *cstr);
    }
}

static inline void string_append_strv(String* str, Str_view str_view) {
    for (size_t idx = 0; idx < str_view.count; idx++) {
        string_append(str, str_view.str[idx]);
    }
}

static inline String string_new_from_cstr(const char* cstr) {
    String string = {0};
    for (int idx = 0; cstr[idx]; idx++) {
        string_append(&string, cstr[idx]);
    }
    return string;
}

static inline String string_new_from_strv(Str_view str_view) {
    String string = {0};
    for (int idx = 0; str_view.str[idx]; idx++) {
        string_append(&string, str_view.str[idx]);
    }
    return string;
}

static inline void string_set_to_zero_len(String* string) {
    vector_set_to_zero_len(string, sizeof(string->buf[0]));
}

// if string is already initialized (but may or may not be empty)
static inline void string_cpy_cstr_inplace(String* string, const char* cstr) {
    if (!cstr) {
        todo();
    }
    size_t cstr_len = strlen(cstr);
    if (!string->buf) {
        *string = string_new_from_cstr(cstr);
        return;
    }
    string_set_to_zero_len(string);
    string_reserve(string, cstr_len);
    for (int idx = 0; cstr[idx]; idx++) {
        string_append(string, cstr[idx]);
    }
}

static inline void string_extend_string(String* dest, const String src) {
    for (size_t idx = 0; idx < src.info.count; idx++) {
        string_append(dest, src.buf[idx]);
    }
}

static inline void string_extend_strv(String* string, Str_view str_view) {
    for (size_t idx = 0; idx < str_view.count; idx++) {
        string_append(string, str_view.str[idx]);
    }
}

static inline void string_add_int(String* string, int num) {
    const char* fmt_str =  " (line %d)";
    static char num_str[20];
    sprintf(num_str, fmt_str, num);
    string_extend_cstr(string, num_str);
}

static inline void string_extend_strv_in_sym(String* string, Str_view str_view, char opening_symbol, char closing_symbol) {
    string_append(string, opening_symbol);
    string_extend_strv(string, str_view);
    string_append(string, closing_symbol);
}

static inline void string_extend_strv_in_par(String* string, Str_view str_view) {
    string_extend_strv_in_sym(string, str_view, '(', ')');
}

static inline void string_extend_strv_in_gtlt(String* string, Str_view str_view) {
    string_extend_strv_in_sym(string, str_view, '<', '>');
}

#endif // NEWSTRING_H
