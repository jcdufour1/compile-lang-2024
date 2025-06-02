#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <util.h>
#include <strv_struct.h>
#include <ctype.h>
#include <arena.h>

static inline Strv strv_slice(Strv strv, size_t start, size_t count) {
    unwrap(count <= strv.count && start + count <= strv.count && "out of bounds");
    Strv new_strv = {.str = strv.str + start, .count = count};
    return new_strv;
}

static inline char strv_at(Strv strv, size_t index) {
    unwrap(index < strv.count && "out of bounds");
    return strv.str[index];
}

static inline char strv_front(Strv strv) {
    return strv_at(strv, 0);
}

static inline Strv strv_consume_while(Strv* strv, bool (*should_continue)(char /* previous char */, char /* current char */)) {
    Strv new_strv;
    for (size_t idx = 0; strv->count > idx; idx++) {
        char prev_char = idx > 0 ? (strv_at(*strv, idx)) : (0);
        if (!should_continue(prev_char, strv_at(*strv, idx))) {
            new_strv.str = strv->str;
            new_strv.count = idx;
            strv->str += idx;
            strv->count -= idx;
            return new_strv;
        }
    }
    unreachable("cond is never met");
}

static inline bool strv_try_consume_until(Strv* result, Strv* strv, char delim) {
    for (size_t idx = 0; idx < strv->count; idx++) {
        if (strv_at(*strv, idx) == delim) {
            result->str = strv->str;
            result->count = idx;
            strv->str += idx;
            strv->count -= idx;
            return true;
        }
    }
    return false;
}

static inline Strv strv_consume_until(Strv* strv, char delim) {
    Strv result = {0};
    unwrap(strv_try_consume_until(&result, strv, delim) && "delim not found");
    return result;
}

static inline Strv strv_consume_count(Strv* strv, size_t count) {
    if (strv->count < count) {
        todo();
    }
    Strv new_strv;
    new_strv.str = strv->str;
    new_strv.count = count;
    strv->str += count;
    strv->count -= count;
    return new_strv;
}

static inline char strv_consume(Strv* strv) {
    return strv_front(strv_consume_count(strv, 1));
}

static inline bool strv_is_equal(Strv a, Strv b) {
    if (a.count != b.count) {
        return false;
    }
    return 0 == strncmp(a.str, b.str, a.count);
}

static inline bool strv_starts_with(Strv base, Strv prefix) {
    return base.count >= prefix.count && strv_is_equal(strv_slice(base, 0, prefix.count), prefix);
}

// only cstrs with a long enough lifetime can be passed here
static inline Strv sv(const char* cstr) {
    return (Strv) {.str = cstr, .count = strlen(cstr)};
}

static inline bool strv_try_consume(Strv* strv, char ch) {
    if (strv->count < 1) {
        return false;
    }
    if (strv_front(*strv) == ch) {
        strv_consume(strv);
        return true;
    }
    return false;
}

static inline bool strv_try_consume_count(Strv* strv, char ch, size_t count) {
    for (size_t idx = 0; idx < count; idx++) {
        if (!strv_try_consume(strv, ch)) {
            return false;
        }
    }
    return true;
}

static inline const char* strv_dup(Arena* arena, Strv strv) {
    return arena_strndup(arena, strv.str, strv.count);
}

static inline const char* strv_to_cstr(Arena* arena, Strv strv) {
    char* buf = arena_alloc(arena, strv.count + 1);
    memcpy(buf, strv.str, strv.count);
    return buf;
}

#define strv_print(strv) (int)((strv).count), (strv).str

#endif // STR_VIEW_H
