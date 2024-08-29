#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "str_view_struct.h"

static inline char str_view_front(Str_view str_view) {
    if (str_view.count < 1) {
        todo();
    }
    return str_view.str[0];
}

static inline Str_view str_view_chop_on_cond(Str_view* str_view, bool (*should_continue)(char /* previous char */, char /* current char */)) {
    Str_view new_str_view;
    for (size_t idx = 0; str_view->count > idx; idx++) {
        char prev_char = idx > 0 ? (str_view->str[idx]) : (0);
        if (!should_continue(prev_char, str_view->str[idx])) {
            new_str_view.str = str_view->str;
            new_str_view.count = idx;
            str_view->str += idx;
            str_view->count -= idx;
            return new_str_view;
        }
    }
    todo();
}

static inline Str_view str_view_chop_on_delim(Str_view* str_view, char delim) {
    Str_view new_str_view;
    for (size_t idx = 0; str_view->count > idx; idx++) {
        if (str_view->str[idx] == delim) {
            new_str_view.str = str_view->str;
            new_str_view.count = idx;
            str_view->str += idx;
            str_view->count -= idx;
            return new_str_view;
        }
    }

    // delim not found
    todo();
}

static inline Str_view str_view_chop_front_count(Str_view* str_view, size_t count) {
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

static inline Str_view str_view_chop_front(Str_view* str_view) {
    return str_view_chop_front_count(str_view, 1);
}

// return 0 when match
static inline int str_view_cmp_cstr(Str_view str_view, const char* cstr) {
    if (strlen(cstr) != str_view.count) {
        return 100;
    }
    return strncmp(str_view.str, cstr, str_view.count);
}

// return 0 when match
static inline int str_view_cmp(Str_view a, Str_view b) {
    if (a.count != b.count) {
        return 100;
    }
    return strncmp(a.str, b.str, a.count);
}

// only string literals can be passed into this function
static inline Str_view str_view_from_cstr(const char* cstr) {
    Str_view str_view;
    str_view.str = cstr;
    str_view.count = strlen(cstr);
    return str_view;
}

#define STR_VIEW_FMT "%.*s"

#define str_view_print(str_view) (int)((str_view).count), (str_view).str

#endif // STR_VIEW_H
