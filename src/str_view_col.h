#ifndef STR_VIEW_COL_H
#define STR_VIEW_COL_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "str_view.h"
#include "str_view_struct.h"

typedef struct {
    Str_view base;
} Str_view_col;

static inline void str_view_col_advance_pos(Pos* pos, char ch) {
    if (ch == '\n') {
        pos->line++;
        pos->column = 0;
    } else {
        pos->column++;
    }
}

static inline char str_view_col_front(Str_view_col str_view) {
    return str_view_front(str_view.base);
}

static inline char str_view_col_consume(Pos* pos, Str_view_col* str_view) {
    str_view_col_advance_pos(pos, str_view_col_front(*str_view));
    return str_view_consume(&str_view->base);
}

static inline bool str_view_col_try_consume_while(
    Str_view_col* result,
    Pos* pos,
    Str_view_col* str_view,
    bool (*should_continue)(char /* previous char */, char /* current char */)
) {
    Str_view* base = &str_view->base;
    for (size_t idx = 0; base->count > idx; idx++) {
        char prev_char = idx > 0 ? (str_view_at(*base, idx)) : (0);
        log(LOG_DEBUG, "thing thign\n"); 
        if (!should_continue(prev_char, str_view_at(*base, idx))) {
            result->base.str = base->str;
            result->base.count = idx;
            base->str += idx;
            base->count -= idx;
            log(LOG_DEBUG, "thing thign 3\n"); 
            return true;
        }
        str_view_col_advance_pos(pos, str_view_at(*base, idx));
    }
    log(LOG_DEBUG, "thing thign 4\n"); 
    return false;
}

static inline Str_view_col str_view_col_consume_while(
    Pos* pos,
    Str_view_col* str_view,
    bool (*should_continue)(char /* previous char */, char /* current char */)
) {
    Str_view_col result = {0};
    if (str_view_col_try_consume_while(&result, pos, str_view, should_continue)) {
        return result;
    }
    unreachable("condition is never met");
}

static inline Str_view_col str_view_col_consume_until(Pos* pos, Str_view_col* str_view, char delim) {
    Str_view_col new_str_view;
    Str_view* base = &str_view->base;
    for (size_t idx = 0; base->count > idx; idx++) {
        if (str_view_at(*base, idx) == delim) {
            new_str_view.base.str = base->str;
            new_str_view.base.count = idx;
            base->str += idx;
            base->count -= idx;
            return new_str_view;
        }
        str_view_col_advance_pos(pos, str_view_at(*base, idx));
    }
    unreachable("delim not found");
}

// return true when match
static inline bool str_view_col_cstr_is_equal(Str_view_col str_view, const char* cstr) {
    return str_view_cstr_is_equal(str_view.base, cstr);
}

static inline int str_view_col_cmp(Str_view_col a, Str_view_col b) {
    return str_view_cmp(a.base, b.base);
}

static inline bool str_view_col_is_equal(Str_view_col a, Str_view_col b) {
    return str_view_is_equal(a.base, b.base);
}

static inline bool str_view_col_try_consume(Pos* pos, Str_view_col* str_view, char ch) {
    if (str_view_try_consume(&str_view->base, ch)) {
        str_view_col_advance_pos(pos, ch);
        return true;
    }
    return false;
}

#define STR_VIEW_COL_FMT STR_VIEW_FMT

#define str_view_col_print(str_view) str_view_print((str_view).base)

#endif // STR_VIEW_COL_H
