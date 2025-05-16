#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "str_view_struct.h"
#include <ctype.h>
#include <arena.h>

static inline Str_view str_view_slice(Str_view str_view, size_t start, size_t count) {
    assert(count <= str_view.count && start + count <= str_view.count && "out of bounds");
    Str_view new_str_view = {.str = str_view.str + start, .count = count};
    return new_str_view;
}

static inline char str_view_at(Str_view str_view, size_t index) {
    assert(index <= str_view.count && "out of bounds");
    return str_view.str[index];
}

static inline char str_view_front(Str_view str_view) {
    return str_view_at(str_view, 0);
}

static inline Str_view str_view_consume_while(Str_view* str_view, bool (*should_continue)(char /* previous char */, char /* current char */)) {
    Str_view new_str_view;
    for (size_t idx = 0; str_view->count > idx; idx++) {
        char prev_char = idx > 0 ? (str_view_at(*str_view, idx)) : (0);
        if (!should_continue(prev_char, str_view_at(*str_view, idx))) {
            new_str_view.str = str_view->str;
            new_str_view.count = idx;
            str_view->str += idx;
            str_view->count -= idx;
            return new_str_view;
        }
    }
    unreachable("cond is never met");
}

static inline bool str_view_try_consume_until(Str_view* result, Str_view* str_view, char delim) {
    for (size_t idx = 0; idx < str_view->count; idx++) {
        if (str_view_at(*str_view, idx) == delim) {
            result->str = str_view->str;
            result->count = idx;
            str_view->str += idx;
            str_view->count -= idx;
            return true;
        }
    }
    return false;
}

static inline Str_view str_view_consume_until(Str_view* str_view, char delim) {
    Str_view result = {0};
    unwrap(str_view_try_consume_until(&result, str_view, delim) && "delim not found");
    return result;
}

static inline Str_view str_view_consume_count(Str_view* str_view, size_t count) {
    if (str_view->count < count) {
        todo();
    }
    Str_view new_str_view;
    new_str_view.str = str_view->str;
    new_str_view.count = count;
    str_view->str += count;
    str_view->count -= count;
    return new_str_view;
}

static inline char str_view_consume(Str_view* str_view) {
    return str_view_front(str_view_consume_count(str_view, 1));
}

// return true when match
static inline bool str_view_cstr_is_equal(Str_view str_view, const char* cstr) {
    if (strlen(cstr) != str_view.count) {
        return false;
    }
    return 0 == strncmp(str_view.str, cstr, str_view.count);
}

static inline int str_view_cmp(Str_view a, Str_view b) {
    if (a.count != b.count) {
        return 100;
    }
    return strncmp(a.str, b.str, a.count);
}

static inline bool str_view_is_equal(Str_view a, Str_view b) {
    return 0 == str_view_cmp(a, b);
}

// only string literals can be passed into this function
static inline Str_view str_view_from_cstr(const char* cstr) {
    Str_view str_view;
    str_view.str = cstr;
    str_view.count = strlen(cstr);
    assert(str_view.count < 1e6);
    return str_view;
}

static inline bool str_view_try_consume(Str_view* str_view, char ch) {
    if (str_view->count < 1) {
        return false;
    }
    if (str_view_front(*str_view) == ch) {
        str_view_consume(str_view);
        return true;
    }
    return false;
}

static inline bool str_view_try_consume_count(Str_view* str_view, char ch, size_t count) {
    for (size_t idx = 0; idx < count; idx++) {
        if (!str_view_try_consume(str_view, ch)) {
            return false;
        }
    }
    return true;
}

static inline const char* str_view_dup(Arena* arena, Str_view str_view) {
    return arena_strndup(arena, str_view.str, str_view.count);
}

#define STR_VIEW_FMT "%.*s"

#define str_view_print(str_view) (int)((str_view).count), (str_view).str

#endif // STR_VIEW_H
