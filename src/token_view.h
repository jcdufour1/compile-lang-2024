#ifndef TOKEN_VIEW_H
#define TOKEN_VIEW_H

#include <stddef.h>
#include "token.h"
#include "assert.h"

typedef struct {
    const Token* tokens;
    size_t count;
} Tk_view;

static inline Token tk_view_at(Tk_view tk_view, size_t idx) {
    assert(tk_view.count > idx);
    return tk_view.tokens[idx];
}

static inline Token tk_view_front(Tk_view tk_view) {
    return tk_view_at(tk_view, 0);
}

#define log_tokens(log_level, tk_view) \
    do { \
        log((log_level), "tokens:\n"); \
        for (size_t idx = 0; idx < (tk_view).count; idx++) { \
            log((log_level), TOKEN_FMT"\n", token_print(TOKEN_MODE_LOG, tk_view_at(tk_view, idx))); \
        } \
        log(log_level, "\n"); \
    } while(0);

static inline Tk_view tk_view_consume_count(Tk_view* tk_view, size_t count) {
    if (tk_view->count < count) {
        unreachable("out of bounds");
    }
    Tk_view result = {.tokens = tk_view->tokens, .count = count};
    tk_view->tokens += count;
    tk_view->count -= count;
    return result;
}

static inline Token tk_view_consume(Tk_view* tk_view) {
    return tk_view_front(tk_view_consume_count(tk_view, 1));
}

static inline bool tk_view_try_consume(Token* result, Tk_view* tokens, TOKEN_TYPE expected) {
    if (tokens->count < 1 || tk_view_front(*tokens).type != expected) {
        return false;
    }
    
    Token token = tk_view_consume(tokens);
    if (result) {
        *result = token;
    }
    assert(result);
    return true;
}

static inline bool tk_view_try_consume_symbol(Token* result, Tk_view* tokens, const char* cstr) {
    if (!str_view_cstr_is_equal(tk_view_front(*tokens).text, cstr)) {
        return false;
    }
    return tk_view_try_consume(result, tokens, TOKEN_SYMBOL);
}

static inline Str_view tk_view_print_internal(Arena* arena, Tk_view tk_view) {
    String buf = {0};
    vec_reset(&buf);

    for (size_t idx = 0; idx < tk_view.count; idx++) {
        string_extend_strv(&print_arena, &buf, token_print_internal(arena, TOKEN_MODE_LOG, tk_view_at(tk_view, idx)));
        string_extend_cstr(&print_arena, &buf, ";    ");
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static inline bool tk_view_is_equal_internal(LOG_LEVEL log_level, Tk_view a, Tk_view b, bool do_log) {
    for (size_t idx = 0; idx < MIN(a.count, b.count); idx++) {
        if (!token_is_equal(tk_view_at(a, idx), tk_view_at(b, idx))) {
            if (do_log) {
                log(log_level, "TOKENS actual:\n");
                log_tokens(log_level, a);
                log(log_level, "TOKENS expected:\n");
                log_tokens(log_level, b);
                log(
                    log_level, "idx %zu: "TOKEN_FMT" is not equal to "TOKEN_FMT"\n",
                    idx, token_print(TOKEN_MODE_LOG, tk_view_at(a, idx)), token_print(TOKEN_MODE_LOG, tk_view_at(b, idx))
                );
            }
            return false;
        }
    }

    if (a.count != b.count) {
        if (do_log) {
            log(log_level, "count is not equal\n");
            log(log_level, "TOKENS actual:\n");
            log_tokens(log_level, a);
            log(log_level, "TOKENS expected:\n");
            log_tokens(log_level, b);
        }
        return false;
    }

    return true;
}

static inline bool tk_view_is_equal(Tk_view a, Tk_view b) {
    return tk_view_is_equal_internal(LOG_TRACE, a, b, false);
}

static inline bool tk_view_is_equal_log(LOG_LEVEL log_level, Tk_view a, Tk_view b) {
    return tk_view_is_equal_internal(log_level, a, b, true);
}

#define TK_VIEW_FMT STR_VIEW_FMT

#define tk_view_print(tk_view) str_view_print(tk_view_print_internal(tk_view))

#endif // TOKEN_VIEW_H
