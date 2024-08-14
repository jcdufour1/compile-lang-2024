#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"

typedef struct {
    const char* str;
    size_t count;
} Str_view;

static inline void Str_view_init(Str_view* str_view) {
    memset(str_view, 0, sizeof(*str_view));
}

static inline char strv_front(Str_view str_view) {
    if (str_view.count < 1) {
        todo();
    }
    return str_view.str[0];
}

static inline Str_view strv_chop_on_cond(Str_view* str_view, bool (*should_continue)(char)) {
    Str_view new_str_view;
    for (size_t idx = 0; str_view->count > idx; idx++) {
        if (!should_continue(str_view->str[idx])) {
            new_str_view.str = str_view->str;
            new_str_view.count = idx;
            str_view->str += idx;
            str_view->count -= idx;
            return new_str_view;
        }
    }
    todo();
}

static inline Str_view strv_chop_on_delim(Str_view* str_view, char delim) {
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

static inline Str_view strv_chop_front_count(Str_view* str_view, size_t count) {
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

static inline Str_view strv_chop_front(Str_view* str_view) {
    return strv_chop_front_count(str_view, 1);
}

// return 0 when match
static inline int Strv_cmp_cstr(Str_view str_view, const char* cstr) {
    return strncmp(str_view.str, cstr, str_view.count);
}

#define STRV_FMT "%.*s"

#define Strv_print(str_view) (int)((str_view).count), (str_view).str

#endif // STR_VIEW_H
