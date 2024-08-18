#ifndef NEWSTRING_H
#define NEWSTRING_H

#include <string.h>
#include "util.h"
#include "str_view.h"

#define STRING_DEFAULT_CAPACITY 4048

typedef struct {
    char* buf;
    size_t count;
    size_t capacity;
} String;

#define STRING_FMT "%.*s"

#define string_print(string) (int)((string).count), (string).buf

static inline void string_reserve(String* str, size_t minimum_count_empty_slots) {
    // str->capacity must be at least one greater than str->count for null termination
    while (str->count + minimum_count_empty_slots + 1 > str->capacity) {
        if (str->capacity < 1) {
            str->capacity = STRING_DEFAULT_CAPACITY;
            str->buf = safe_malloc(str->capacity, sizeof(str->buf[0]));
        } else {
            str->capacity *= 2;
            str->buf = safe_realloc(str->buf, str->capacity, sizeof(str->buf[0]));
        }
    }
}

// string->buf is always null terminated
static inline void string_append(String* str, char ch) {
    string_reserve(str, 1);
    str->buf[str->count++] = ch;
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

static inline void string_set_to_zero_len(String* string) {
    if (string->count < 1) {
        return;
    }
    memset(string->buf, 0, sizeof(string->buf[0])*string->count);
    string->count = 0;
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
    for (size_t idx = 0; idx < src.count; idx++) {
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

#endif // NEWSTRING_H
