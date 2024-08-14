#ifndef NEWSTRING_H
#define NEWSTRING_H

#include <string.h>
#include "util.h"

#define STRING_DEFAULT_CAPACITY 4048

typedef struct {
    char* buf;
    size_t count;
    size_t capacity;
} String;

static void String_init(String* str) {
    memset(str, 0, sizeof(*str));
}

#define STRING_FMT "%.*s"

#define String_print(string) (int)((string).count), (string).buf

static inline void String_reserve(String* str, size_t minimum_count_empty_slots) {
    // str->capacity must be at least one greater than str->count for null termination
    while (str->count + minimum_count_empty_slots + 1 > str->capacity) {
        if (str->capacity < 1) {
            str->capacity = STRING_DEFAULT_CAPACITY;
            str->buf = safe_malloc(str->capacity);
        } else {
            str->capacity *= 2;
            str->buf = safe_realloc(str->buf, str->capacity);
        }
    }
}

// string->buf is always null terminated
static inline void String_append(String* str, char ch) {
    String_reserve(str, 1);
    str->buf[str->count++] = ch;
}

static inline String String_from_cstr(const char* cstr) {
    String string;
    String_init(&string);
    for (int idx = 0; cstr[idx]; idx++) {
        String_append(&string, cstr[idx]);
    }
    return string;
}

#endif // NEWSTRING_H
